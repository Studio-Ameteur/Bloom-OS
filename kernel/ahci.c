#include "ahci.h"
#include "pmm.h"

#pragma pack(push, 1)

typedef struct {
    volatile uint32_t Clb;
    volatile uint32_t Clbu;
    volatile uint32_t Fb;
    volatile uint32_t Fbu;
    volatile uint32_t Is;
    volatile uint32_t Ie;
    volatile uint32_t Cmd;
    volatile uint32_t Reserved0;
    volatile uint32_t Tfd;
    volatile uint32_t Sig;
    volatile uint32_t Ssts;
    volatile uint32_t Sctl;
    volatile uint32_t Serr;
    volatile uint32_t Sact;
    volatile uint32_t Ci;
    volatile uint32_t Sntf;
    volatile uint32_t Fbs;
    volatile uint32_t Reserved1[11];
    volatile uint32_t Vendor[4];
} HBA_PORT;

typedef struct {
    volatile uint32_t Cap;
    volatile uint32_t Ghc;
    volatile uint32_t Is;
    volatile uint32_t Pi;
    volatile uint32_t Vs;
    volatile uint32_t CccCtl;
    volatile uint32_t CccPorts;
    volatile uint32_t EmLoc;
    volatile uint32_t EmCtl;
    volatile uint32_t Cap2;
    volatile uint32_t Bohc;
    uint8_t Reserved[0xA0 - 0x2C];
    uint8_t Vendor[0x100 - 0xA0];
    HBA_PORT Ports[32];
} HBA_MEM;

typedef struct {
    uint8_t Cfl : 5;
    uint8_t Atapi : 1;
    uint8_t Write : 1;
    uint8_t Prefetchable : 1;
    uint8_t Reset : 1;
    uint8_t Bist : 1;
    uint8_t ClearBusy : 1;
    uint8_t Reserved0 : 1;
    uint8_t Pmp : 4;
    uint16_t Prdtl;
    volatile uint32_t Prdbc;
    uint32_t Ctba;
    uint32_t Ctbau;
    uint32_t Reserved1[4];
} HBA_CMD_HEADER;

typedef struct {
    uint32_t Dba;
    uint32_t Dbau;
    uint32_t Reserved0;
    uint32_t Dbc : 22;
    uint32_t Reserved1 : 9;
    uint32_t InterruptOnCompletion : 1;
} HBA_PRDT_ENTRY;

typedef struct {
    uint8_t Cfis[64];
    uint8_t Acmd[16];
    uint8_t Reserved[48];
    HBA_PRDT_ENTRY PrdtEntry[1];
} HBA_CMD_TBL;

typedef struct {
    uint8_t FisType;
    uint8_t PmPort : 4;
    uint8_t Reserved0 : 3;
    uint8_t CommandFlag : 1;
    uint8_t Command;
    uint8_t FeatureLow;
    uint8_t Lba0;
    uint8_t Lba1;
    uint8_t Lba2;
    uint8_t Device;
    uint8_t Lba3;
    uint8_t Lba4;
    uint8_t Lba5;
    uint8_t FeatureHigh;
    uint8_t CountLow;
    uint8_t CountHigh;
    uint8_t Icc;
    uint8_t Control;
    uint8_t Reserved1[4];
} FIS_REG_H2D;

#pragma pack(pop)

#define AHCI_GHC_AE (1u << 31)
#define AHCI_CMD_ST (1u << 0)
#define AHCI_CMD_FRE (1u << 4)
#define AHCI_CMD_FR (1u << 14)
#define AHCI_CMD_CR (1u << 15)
#define AHCI_TFD_BSY 0x80
#define AHCI_TFD_DRQ 0x08
#define ATA_CMD_READ_DMA_EXT 0x25

static HBA_PORT *ActivePort;
static HBA_CMD_TBL *ActiveCmdTable;

static void
StopCommandEngine(HBA_PORT *Port)
{
    Port->Cmd &= ~AHCI_CMD_ST;
    Port->Cmd &= ~AHCI_CMD_FRE;

    while (Port->Cmd & (AHCI_CMD_CR | AHCI_CMD_FR)) {
    }
}

static void
StartCommandEngine(HBA_PORT *Port)
{
    while (Port->Cmd & AHCI_CMD_CR) {
    }

    Port->Cmd |= AHCI_CMD_FRE;
    Port->Cmd |= AHCI_CMD_ST;
}

int
AhciInit(uint32_t Abar)
{
    HBA_MEM *Hba = (HBA_MEM *)(uintptr_t)Abar;
    Hba->Ghc |= AHCI_GHC_AE;

    HBA_PORT *FoundPort = 0;

    for (int i = 0; i < 32; i++) {
        if (!(Hba->Pi & (1u << i))) {
            continue;
        }

        HBA_PORT *Port = &Hba->Ports[i];
        uint8_t Det = (uint8_t)(Port->Ssts & 0x0F);
        uint8_t Ipm = (uint8_t)((Port->Ssts >> 8) & 0x0F);

        if (Det == 3 && Ipm == 1) {
            FoundPort = Port;
            break;
        }
    }

    if (!FoundPort) {
        return 0;
    }

    StopCommandEngine(FoundPort);

    void *ClbPage = AllocPage();
    void *FbPage = AllocPage();
    void *CmdTablePage = AllocPage();

    if (!ClbPage || !FbPage || !CmdTablePage) {
        return 0;
    }

    uint8_t *ClbBytes = (uint8_t *)ClbPage;
    for (uint64_t i = 0; i < 4096; i++) {
        ClbBytes[i] = 0;
    }

    uint8_t *FbBytes = (uint8_t *)FbPage;
    for (uint64_t i = 0; i < 4096; i++) {
        FbBytes[i] = 0;
    }

    uint8_t *CmdTableBytes = (uint8_t *)CmdTablePage;
    for (uint64_t i = 0; i < 4096; i++) {
        CmdTableBytes[i] = 0;
    }

    FoundPort->Clb = (uint32_t)(uintptr_t)ClbPage;
    FoundPort->Clbu = (uint32_t)((uintptr_t)ClbPage >> 32);
    FoundPort->Fb = (uint32_t)(uintptr_t)FbPage;
    FoundPort->Fbu = (uint32_t)((uintptr_t)FbPage >> 32);

    HBA_CMD_HEADER *CmdHeader = (HBA_CMD_HEADER *)ClbPage;
    CmdHeader[0].Ctba = (uint32_t)(uintptr_t)CmdTablePage;
    CmdHeader[0].Ctbau = (uint32_t)((uintptr_t)CmdTablePage >> 32);

    StartCommandEngine(FoundPort);

    ActivePort = FoundPort;
    ActiveCmdTable = (HBA_CMD_TBL *)CmdTablePage;

    return 1;
}

int
AhciReadSectors(uint64_t Lba, uint32_t Count, void *Buffer)
{
    if (!ActivePort) {
        return 0;
    }

    while (ActivePort->Tfd & (AHCI_TFD_BSY | AHCI_TFD_DRQ)) {
    }

    HBA_PORT *Port = ActivePort;
    HBA_CMD_HEADER *CmdHeader = (HBA_CMD_HEADER *)(uintptr_t)((uint64_t)Port->Clbu << 32 | Port->Clb);

    CmdHeader[0].Cfl = sizeof(FIS_REG_H2D) / 4;
    CmdHeader[0].Write = 0;
    CmdHeader[0].Prdtl = 1;
    CmdHeader[0].Prdbc = 0;

    HBA_CMD_TBL *CmdTable = ActiveCmdTable;

    for (uint64_t i = 0; i < sizeof(HBA_CMD_TBL); i++) {
        ((uint8_t *)CmdTable)[i] = 0;
    }

    CmdTable->PrdtEntry[0].Dba = (uint32_t)(uintptr_t)Buffer;
    CmdTable->PrdtEntry[0].Dbau = (uint32_t)((uintptr_t)Buffer >> 32);
    CmdTable->PrdtEntry[0].Dbc = (Count * 512) - 1;
    CmdTable->PrdtEntry[0].InterruptOnCompletion = 0;

    FIS_REG_H2D *Fis = (FIS_REG_H2D *)CmdTable->Cfis;
    Fis->FisType = 0x27;
    Fis->CommandFlag = 1;
    Fis->Command = ATA_CMD_READ_DMA_EXT;
    Fis->Device = 1 << 6;

    Fis->Lba0 = (uint8_t)(Lba & 0xFF);
    Fis->Lba1 = (uint8_t)((Lba >> 8) & 0xFF);
    Fis->Lba2 = (uint8_t)((Lba >> 16) & 0xFF);
    Fis->Lba3 = (uint8_t)((Lba >> 24) & 0xFF);
    Fis->Lba4 = (uint8_t)((Lba >> 32) & 0xFF);
    Fis->Lba5 = (uint8_t)((Lba >> 40) & 0xFF);

    Fis->CountLow = (uint8_t)(Count & 0xFF);
    Fis->CountHigh = (uint8_t)((Count >> 8) & 0xFF);

    Port->Is = 0xFFFFFFFF;
    Port->Ci |= (1u << 0);

    while (Port->Ci & (1u << 0)) {
        if (Port->Is & (1u << 30)) {
            return 0;
        }
    }

    return 1;
}
