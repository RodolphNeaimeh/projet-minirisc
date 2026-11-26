#include <stdlib.h>
#include <stdio.h>
#include "minirisc.h"


minirisc_t* minirisc_new(uint32_t initial_PC, platform_t *platform)
{
    minirisc_t *mr = calloc(1, sizeof *mr);
    if (mr == NULL) {
        return NULL;
    }

    mr->PC = initial_PC;
    mr->next_PC = initial_PC;
    mr->platform = platform;

    return mr;
}

void minirisc_free(minirisc_t *mr)
{
    free(mr);
}


void minirisc_fetch(minirisc_t *mr)
{
    // Lit le mot à l'adresse PC et place l'instruction dans IR.
    if (platform_read(mr->platform, PLATFORM_ACCESS_WORD,
                      mr->PC, &mr->IR) != 0) {
        mr->error = 1;
        mr->halt = 1;
    }
}


enum {
    MR_OPCODE_LUI = 0x01,
    MR_OPCODE_AUIPC = 0x02,
    MR_OPCODE_BEQ = 0x05,
    MR_OPCODE_ADDI = 0x13,
    MR_OPCODE_EBREAK = 0x27
};

static inline unsigned int mr_get_rs1num(const minirisc_t *mr)
{
    return (mr->IR >> 12) & 0x1fu;
}

static inline unsigned int mr_get_rs2num_SB(const minirisc_t *mr)
{
    return (mr->IR >> 7) & 0x1fu;
}

static inline int32_t mr_get_imm_IS(const minirisc_t *mr)
{
    int32_t imm = (int32_t)(mr->IR >> 20);

    if (imm >= 2048) {
        imm -= 4096;
    }

    return imm;
}

static inline int32_t mr_get_imm_B(const minirisc_t *mr)
{
    return mr_get_imm_IS(mr) * 2;
}

static inline void mr_write_rd(minirisc_t *mr, uint32_t value)
{
    unsigned int rd = (mr->IR >> 7) & 0x1fu;

    if (rd != 0) {
        mr->regs[rd] = value;
    }
}

static inline uint32_t mr_get_imm_U(const minirisc_t *mr)
{
    return mr->IR & UINT32_C(0xfffff000);
}

void minirisc_decode_and_execute(minirisc_t *mr)
{
    switch (mr->IR & 0x7fu) {

    case MR_OPCODE_LUI:
        mr_write_rd(mr, mr_get_imm_U(mr));
        return;

    case MR_OPCODE_AUIPC:
        mr_write_rd(mr, mr->PC + mr_get_imm_U(mr));
        return;

    case MR_OPCODE_ADDI: {
        uint32_t source = mr->regs[mr_get_rs1num(mr)];
        mr_write_rd(mr, source + (uint32_t)mr_get_imm_IS(mr));
        return;
    }

    case MR_OPCODE_BEQ: {
        unsigned int rs1 = mr_get_rs1num(mr);
        unsigned int rs2 = mr_get_rs2num_SB(mr);

        if (mr->regs[rs1] == mr->regs[rs2]) {
            mr->next_PC = mr->PC + (uint32_t)mr_get_imm_B(mr);
        }
        return;
    }

    case MR_OPCODE_EBREAK:
        if (mr->IR == UINT32_C(0x00000027)) {
            mr->halt = 1;
            return;
        }
        break;

    default:
        break;
    }

    fprintf(stderr, "Instruction non prise en charge : 0x%08" PRIx32 "\n",
            mr->IR);
    mr->error = 1;
    mr->halt = 1;
}

int minirisc_run(minirisc_t *mr)
{
    while (!mr->halt) {
        minirisc_fetch(mr);
        if (mr->halt) {
            break;
        }

        mr->next_PC = mr->PC + 4u;
        minirisc_decode_and_execute(mr);

        if (!mr->halt) {
            mr->PC = mr->next_PC;
        }
    }

    return mr->error ? -1 : 0;
}

