#ifndef UI_SIM_H
#define UI_SIM_H

#include <stdint.h>

typedef struct {
    const char *name;
    float vi, ii, vb, ib, ic, id, vl;
    uint8_t pc, pd;
    uint8_t soc, soh;
    uint8_t fc, fd;
    uint8_t it, cy;
    uint16_t qm;
    const char *ela;
    const char *eta;
    uint8_t ph;
} sim_state_t;

typedef struct {
    uint8_t  ich;    /* charge current step (×0.5 A) */
    uint8_t  idh;    /* discharge current step (×0.5 A) */
    uint16_t vfl;    /* full voltage ×100 V */
    uint16_t vcu;    /* cutoff voltage ×100 V */
    uint8_t  cycl;   /* learn cycles */
    uint8_t  rest;   /* rest hours ×0.5 */
    uint8_t  cells;  /* cells - 1 (1S = 0) */
} sim_cfg_t;

#define SIM_STATE_COUNT 6
extern const sim_state_t sim_states[SIM_STATE_COUNT];
extern sim_cfg_t         sim_cfg;

#endif
