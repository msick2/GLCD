# 충방전 학습 보드 설계 가이드

작성: 2026-04-10

## 1. 프로젝트 개요

BQ34Z100 같은 퓨얼게이지 IC의 학습 사이클(Capacity learning, OCV/RA/QMAX 갱신)을 자동 수행하는 정밀 충방전 보드.

| 항목 | 내용 |
|---|---|
| 입력 | **DC 48 V 고정** (외부 어댑터) |
| 출력 1 (충전) | **3S~6S Li-ion** (12.6~25.2 V), 0~5 A (typical 2 A) |
| 출력 2 (방전) | dump load 2.5 Ω 40 W (10 Ω 10 W × 4 병렬), 0~4 A safe (typical 2 A) |
| 사이클 | CC charge → CV → REST → CC discharge → REST → 반복 |
| BMS 통신 | I²C, BQ34Z100 별도 보드 |
| UI | 256×160 4-gray STN LCD (구현 완료) |
| 측정 정밀도 목표 | 전압 10 mV, 전류 10 mA reproducible |

---

## 2. 시스템 블록도

```
                ┌────────────────────────────────────────────┐
   48 V ────┬──►│ 충전 벅 (비동기, 85 kHz, HS NMOS)          ├────► VBAT
            │   └────────────────────────────────────────────┘        │
            │            ▲                                            │
            │            │ duty                                       │
   IIN     ▼            │                                            ▼
   Hall ─────────┐   ┌───────────┐         I_CHG Hall ────────► 배터리
                 │   │   RP2040  │                              (1S~7S)
                 ▼   │  /RP2350  │                                    │
            ADC ────►│ Core 0:RT │◄──── ADC 6 ch (AD7606)             │
            (AD7606) │ Core 1:UI │                                    │
                     └───────────┘                                    │
                          │   ▲                                       │
                          │   │ I²C                                   │
                          │   │                                       │
              SPI0 ───────┤   └────► BQ34Z100 (별도 보드)              │
                          │                                           │
                          ▼                                           ▼
               JLX256160G LCD             ┌─────────────────────┐
               (4-gray UI, 10 Hz)         │ 방전 벅 (비동기,    │
                                          │ 85 kHz, HS NMOS)    │
                                          └─────────────────────┘
                                                    │
                                                    ▼
                                          dump load (1 Ω 50 W)
                                                    │
                                                    │
                                          I_DCHG Hall ─── ADC
```

---

## 3. MCU 선정 ✅ Raspberry Pi Pico 2 (RP2350) 확정

| 항목 | 값 |
|---|---|
| 보드 | Raspberry Pi Pico 2 |
| 칩 | RP2350 |
| 코어 | Dual Cortex-M33 @ 150 MHz (300 MHz 부스트 가능) |
| FPU | **있음** (FPv5-SP, 단정도) — float 1~14 cycle |
| SRAM | 520 KB |
| 플래시 | 4 MB |
| PWM 슬라이스 | 12 |
| GPIO | 30 (RP2040과 동일) |
| ADC | 4 ch + 온도 (내장, 사용 안 함) |
| SDK | Pico SDK 2.x 필요 |

### 업그레이드 영향
- 현재 LCD/UI 코드는 HAL 분리 덕분에 거의 그대로 작동
- `CMakeLists.txt`에서 `set(PICO_BOARD pico2)` + `PICO_PLATFORM rp2350` 변경
- pico-setup-windows v1.5.1 → SDK 2.x 재설치 (VS Code Pico 확장 권장)
- 핀 매핑은 RP2040과 거의 동일

### Pico 2의 장점 (이 프로젝트 관점)
- **컨트롤 루프 float 자유**: PI/PID 계산이 1~14 cycle, 8.5 kHz 컨트롤 루프 CPU 사용률 < 5%
- **더 큰 SRAM**: 측정 ring buffer, 학습 로그, 큰 폰트 등 여유
- **NVIC 256 우선순위**: 안전 인터럽트(OCP)를 컨트롤 루프 위에 깔끔하게 배치
- **향후 확장**: 셀 수 증가, 채널 수 증가, 알고리즘 복잡화 모두 대응

---

## 4. 듀얼 코어 분담 (확정)

**Core 0 = bare-metal (RT)**, **Core 1 = FreeRTOS (UI/Comms)**

```
┌──────────────────────────┐  shared mem  ┌──────────────────────────────┐
│ Core 0 (bare-metal RT)   │ ◄══════════► │ Core 1 (FreeRTOS scheduler)  │
│                          │  snapshot    │                              │
│ • PWM HW (85 kHz)        │              │ • lcd_task    (10 Hz)        │
│ • ADC DMA + 평균         │              │ • bms_task    (1 Hz, BQ I²C) │
│ • 8.5 kHz 컨트롤 ISR     │  command Q   │ • settings_task (EEPROM I²C) │
│ • PI + CC/CV 모드 전환   │              │ • input_task    (button)     │
│ • 안전 (OCP/OVP)         │              │ • learn_task    (학습 머신)   │
│ • 하드 OCP NMI           │              │ • uart_task     (외부 통신)   │
│                          │              │ • log_task      (USB CDC)    │
└──────────────────────────┘              └──────────────────────────────┘
   결정적 (jitter 0)                          유연 (RTOS scheduler)
```

### Core 0 특징
- **FreeRTOS API 호출 금지** (RTOS는 Core 1만 인지)
- 타이머 인터럽트로 8.5 kHz 컨트롤 루프 직접 실행
- ADC DMA + ring buffer는 백그라운드 자동
- super-loop 없음, `__wfi()`로 인터럽트 대기
- 결정성 최상 (RTOS scheduler 오버헤드 0)

### Core 1 특징
- FreeRTOS scheduler가 모든 task 관리
- `configNUMBER_OF_CORES = 1` (단일 코어 모드)
- 6~7개 task로 동시 작업 분리 (LCD, BMS, EEPROM, 학습, UART, USB, 입력)
- 각 task는 `vTaskDelay`, `xQueueReceive(timeout)` 등으로 blocking 가능
- 학습 사이클 같은 긴 blocking이 task로 자연스럽게 표현됨

### 데이터 교환 방식

**측정 스냅샷** (Core 0 → Core 1, 8.5 kHz 갱신):
- 더블 버퍼 + atomic 인덱스 swap
- Core 0가 inactive 버퍼에 채우고 인덱스 swap
- Core 1 task가 필요할 때 active 인덱스의 버퍼만 read

**명령 큐** (Core 1 → Core 0):
- `multicore_fifo` (8 entry HW FIFO) 또는 spinlock-protected ring buffer
- 명령 종류: SET_MODE, SET_I_CHG, SET_V_FULL, SET_I_DCHG, START_LEARN, STOP

### FreeRTOS 도입 시점
- **Phase 5 (컨트롤 루프 + 듀얼 코어 셋업)** 에서 도입
- Pico 1 / Pico 2 모두 동일 task 코드 (port만 자동 선택)
- MKV31 이식 시에도 task 코드 그대로 (port 변경만)

### 금지 사항
- Core 0에서 `printf`, `malloc`, LCD/I²C 접근 금지 (지연/jitter 유발)
- Core 0에서 FreeRTOS API 호출 금지
- 공유 가변 데이터는 반드시 atomic 또는 double-buffer
- DMA 채널 분리 (ADC DMA는 Core 0 전용, LCD/EEPROM/UART DMA는 Core 1 전용)

---

## 5. 측정 채널 (6채널)

| ch | 신호 | 범위 | 분해능 목표 | Front-end |
|---|---|---|---|---|
| 0 | VIN (입력) | 0~48 V | 10 mV | 5:1 분압 (1 MΩ + 250 kΩ) |
| 1 | IIN (입력 전류) | 0~10 A (사용 0~5 A) | 5 mA | Hall ACS725-10AU + RC LPF, 5V 공급, 0.5V offset |
| 2 | VBAT (배터리) | 0~30 V (사용 9~25.2 V, 3S~6S) | 10 mV | 3:1 분압 |
| 3 | I_CHG (충전 전류) | 0~10 A (사용 0~5 A) | 5 mA | Hall ACS725-10AU + RC LPF |
| 4 | I_DCHG (방전 전류) | 0~10 A (사용 0~4 A safe) | 5 mA | Hall ACS725-10AU + RC LPF |
| 5 | VLOAD (부하 전압) | 0~25 V | 10 mV | 3:1 분압 |
| 6 | (예비) | T_FET 등 | — | 필요 시 추가 |
| 7 | (예비) | — | — | — |

**참고**: 배터리 온도는 BQ34Z100 보드의 자체 NTC가 측정 → I²C로 받음. 이 보드에는 NTC 회로 없음. FET 방열판 온도 모니터가 필요하면 ch6에 NTC + 분압 추가 가능.

### 분압 저항 권장
- 정밀도 0.1% 박막 (또는 매칭된 1% 페어)
- VIN 5:1 → 예: R1=4×100k(직렬), R2=100k → 일반 100kΩ × 5
- VBAT/VLOAD 3:1 → 예: R1=200k, R2=100k
- 출력단에 1 nF caps 추가로 RC LPF + ADC sample 안정화
- AD7606 입력 임피던스 ~1 MΩ 이상이라 직접 구동 가능 (버퍼 op-amp 생략)

---

## 6. ADC: AD7606

| 항목 | 값 |
|---|---|
| 분해능 | 16-bit |
| 채널 수 | 8 (6 사용, 2 예비) |
| 샘플링 | **Simultaneous** (각 채널 독립 T/H) |
| 속도 | 200 kSPS / 채널 |
| 입력 범위 | ±10 V (소프트웨어 선택, ±5 V도 가능) |
| 인터페이스 | SPI 또는 parallel — SPI 사용 |
| 트리거 | CONVST 핀 (PWM과 동기 가능) |
| BUSY 핀 | 변환 완료 신호 |
| 가격 | ~$15 |

### 왜 AD7606인가
- ±10 V 직접 입력 → 분압비 작아도 됨 → 저항 매칭 오차 최소화
- 모든 채널 동일 시점 샘플링 → V × I 곱(전력) 정확
- 입력 임피던스 1 MΩ → 외부 버퍼 op-amp 불필요
- 산업 표준, 라이브러리/예제 풍부
- Hall 센서가 ground-referenced 신호라 PGA 불필요 (ADS131M06의 24-bit이 무용지물)

### 채널 사용 분담 (PGA 1× 균등)
- 4 voltage 채널: 분압된 0~10 V
- 2 current 채널: Hall 0~2.2 V (Vref 5 V 모드 사용)

---

## 7. 전류 센서: Allegro ACS725LLCTR-10AU-S (확정)

| 항목 | 값 |
|---|---|
| 부품 번호 | **ACS725LLCTR-10AU-S** |
| 측정 범위 | 0~10 A 단방향 (unipolar) |
| 감도 | **200 mV/A** |
| 출력 (0 A) | ~0.5 V (offset, GND 아님) |
| 출력 (5 A) | ~1.5 V |
| 출력 (10 A) | ~2.5 V |
| **공급전압** | **5 V** (3.3 V 아님 — 5 V rail 필요) |
| 정확도 | ±1.5 % typical |
| 대역폭 | 250 kHz (85 kHz PWM 리플 자연 평균화) |
| 절연 | 1080 Vrms reinforced |
| 응답 시간 | ~4 µs |
| 패키지 | SOIC-8 |
| 가격 | ~$3~4 |

3개 사용: IIN, I_CHG, I_DCHG. 각각 별개 센서.

### 5 V rail 추가 필요
ACS725는 3.3 V로 동작하지 않으므로 보드에 5 V rail이 새로 필요합니다. 옵션:
- 12 V (게이트 드라이버 공급) → 5 V LDO (예: AP2112K-5.0, MCP1700-5002)
- 48 V → 5 V 별도 step-down 컨버터 (MCP16331, TPS54331 등)
- Hall 센서 3개 총 ~30 mA 소비, LDO 충분

### 0.5 V offset 처리
ACS725 출력은 GND가 아닌 0.5 V offset에서 시작 (`Vout = 0.5 + 0.2 × I`). 소프트웨어 캘리브레이션에서:

```c
float current = (adc_voltage - hall_zero_offset) / 0.200f;  /* 0.200 V/A */
/* hall_zero_offset 은 부팅 시 0 A 상태에서 측정하여 보정 */
```

### 분해능 (계산)
- AD7606 16-bit, ±10 V 모드: LSB = 305 µV
- Hall 감도 200 mV/A → LSB = 305 / 200 = **1.53 mA per LSB**
- 5A 측정 시 출력 swing 1.0 V → 약 3,277 codes
- 노이즈 평균화 (10 sample) 적용 시 sub-mA reproducibility

### 대안 (참고용, 사용 안 함)
| 부품 | 범위 | 정확도 | 비고 |
|---|---|---|---|
| **ACS725-10AU** ⭐ | 0~10 A | 1.5% | **확정** |
| ACS70331-005 | 0~5 A | 1.5% | 3.3 V 단전원, 사용 안 함 |
| MLX91220 | ±5 A | 1% | 양방향 |
| ACS37800 | ±30 A | 1% | 디지털 I²C |

---

## 8. 벅 컨버터 설계

### 8.1 충전 벅

| 항목 | 값 |
|---|---|
| 토폴로지 | **비동기 step-down (HS NMOS + Schottky diode)** ⭐ 확정 |
| 입력 | 48 V DC |
| 출력 | **9.0~25.2 V (3S~6S Li-ion)** |
| 출력 전류 max | 5 A |
| 출력 전류 typ | 2 A |
| 스위칭 | 85 kHz |
| Duty 범위 | **19% (3S 9.0/48) ~ 53% (6S 25.2/48)** |
| 효율 목표 | >90% |

### 8.2 방전 벅

| 항목 | 값 |
|---|---|
| 토폴로지 | 비동기 step-down |
| 입력 | VBAT (4.2~29.4 V) |
| 출력 | **dump load 2.5 Ω 40 W** (10 Ω 10 W × 4 병렬) |
| 출력 전류 안전 한계 | **4 A** (P_max = 40 W → I_max = √(40/2.5)) |
| 출력 전류 typ | 2 A (P_load = 10 W, 여유 충분) |
| 스위칭 | 85 kHz |
| Duty 범위 | 부하/배터리 비율에 따름 |

#### 셀 구성별 방전 한계 (2.5 Ω 부하, 3S~6S 대상)

| 셀 | VBAT max | 전기적 max I = V/R | 안전 한계 (40 W) | 실용 한계 |
|---|---|---|---|---|
| **3S** | 12.6 V | 5.04 A | 4 A | **4 A** |
| **4S** | 16.8 V | 6.72 A | 4 A | 4 A |
| **5S** | 21.0 V | 8.40 A | 4 A | 4 A |
| **6S** | 25.2 V | 10.08 A | 4 A | 4 A |

→ 모든 대상 셀에서 **최대 4 A까지** 방전 가능. 2 A 사용 시 부하 전력 10 W로 충분히 여유.

→ 소프트웨어 클램프: `I_dchg_max = min(VBAT/2.5, 4.0) [A]`. 3S에서 VBAT가 9 V로 떨어진 경우 max = 3.6 A.

### 8.3 비동기 토폴로지의 의미

- HS FET 1개만 능동, LS는 Schottky 다이오드
- LS FET 없음 → 데드타임 / shoot-through 걱정 없음
- 음의 인덕터 전류 불가 (단방향) — 충/방전 각각 단방향이라 OK
- 다이오드 손실: `Vf × I_avg × (1−D)`. 5 A에서 약 1 W → Schottky 발열 고려
- 경부하 시 자동 DCM 진입 → 컨트롤 게인 비선형, 1 kHz~8.5 kHz PI는 안정적

### 8.4 핵심 부품

| 역할 | 부품 (확정 또는 후보) | 비고 |
|---|---|---|
| HS FET | NMOS, Vds ≥ 80 V, Rds(on) ≤ 30 mΩ, Qg 낮음 | 후보: BSC059N08NS5, IPB180N10S4 |
| **게이트 드라이버** | **NCP51313ADR2G** ⭐ | ON Semi, 부트스트랩 내장 |
| Schottky 다이오드 | Vrrm ≥ 80 V, If ≥ 5 A | 후보: SBR10U200, MBRB1080 |
| 인덕터 | 100 µH, Isat ≥ 7 A, DCR ≤ 30 mΩ | Würth 7447789100 등 |
| 입력 캐패시터 | 470 µF 100 V 알루미늄 + 10 µF MLCC × 2 | ESR 낮을수록 좋음 |
| 출력 캐패시터 | 470 µF 50 V + 10 µF MLCC × 2 | 출력 리플 ↓ |
| Hall 센서 위치 | 출력 인덕터 직후 (배터리 + 단자 직전) | 평균 전류 측정 |

#### NCP51313ADR2G 메모 (확정)

- **HS only 게이트 드라이버**, 부트스트랩 내장
- 비동기식 토폴로지로 확정 — HS NMOS + Schottky diode
- 데드타임/shoot-through 관리 불필요 (LS 없음)
- 다이오드 손실 1 W 정도 (5 A에서) — Schottky 발열 관리 필요

→ 충전 벅, 방전 벅 모두 동일하게 NCP51313 + 외부 NMOS + Schottky 구성.

### 8.5 인덕터 선정 (3S~6S 기준)

ΔIL = Vout × (1 − D) / (L × fsw)

목표 리플 = 30% × Iout = 30% × 5 A = 1.5 A

**6S (25.2 V) worst case**, D = 0.525, fsw = 85 kHz:
L = 25.2 × 0.475 / (1.5 × 85k) = **94 µH** → **100 µH** 사용

**3S (12.6 V)**, D = 0.263, fsw = 85 kHz:
L = 12.6 × 0.737 / (1.5 × 85k) = 73 µH → 100 µH면 리플 1.09 A (여유)

100 µH 7 A Isat 인덕터 (예: Würth 7447789100)

### 8.6 출력 캐패시터 (대략)

ΔV = ΔIL / (8 × fsw × C) + ΔIL × ESR

목표 리플 50 mV, ΔIL = 1.5 A:
C ≥ 1.5 / (8 × 85k × 0.05) = 44 µF → **470 µF** 알루미늄 + MLCC 병렬

---

## 9. PWM 설계

| 항목 | 값 |
|---|---|
| 주파수 | 85 kHz |
| MCU 클럭 | 125 MHz (RP2040) 또는 150 MHz (RP2350) |
| 카운터 분해능 | `125M / 85k = 1,471` (≈ 10.5 bit) |
| Duty 분해능 | ~0.07 % per step |
| 슬라이스 | 2개 독립 (충전, 방전) |
| 동기화 | PWM 카운터 wrap → ADC CONVST 트리거 |
| 모드 | edge-aligned (기본) 또는 center-aligned |

### RP2040/RP2350 PWM 구성
- 슬라이스 0~7 (RP2040) 또는 0~11 (RP2350)
- 각 슬라이스는 A/B 채널, divisor 16-bit, top 16-bit
- top = 1471 → 0~1470 카운트 → 85.0 kHz
- duty = level 값 (0~1471)
- HW에서 자동 토글, CPU 개입 불필요

### Soft-start
- 시작 시 duty = 0
- 매 컨트롤 tick마다 duty 또는 setpoint를 천천히 ramp up
- 인덕터 inrush, 캐패시터 charge surge 방지

---

## 10. 컨트롤 루프

### 10.1 계층 구조

```
0.1 Hz   학습 사이클 머신 (CC → CV → REST → CC discharge → REST)
   ▼     (Core 1, UI 페이지)
10 Hz    UI 갱신 + BQ34Z100 폴링
   ▼     (Core 1)
8.5 kHz  컨트롤 루프 (PI × 2 벅)
   ▼     (Core 0, 타이머 ISR)
85 kHz   ADC 샘플 + PWM 스위칭
         (HW DMA + PWM 자동, CPU 개입 없음)
```

### 10.2 컨트롤 루프 (8.5 kHz tick)

```
1. ADC ring buffer에서 최근 10 샘플 평균 (각 채널)
2. mode 결정 (CC vs CV)
3. PI step 계산 (충전 + 방전)
4. 안전 체크 (OCP/OVP/OTP)
5. PWM duty 갱신
6. 측정 스냅샷 publish
```

### 10.3 PI 컨트롤러 (float, 충전 벅 예시)

```c
typedef struct {
    float kp, ki;
    float integ;
    float lim_hi, lim_lo;
    float setpoint;
    int   mode;  /* 0=CC, 1=CV */
} pi_t;

float pi_step(pi_t *pi, float feedback) {
    float error = pi->setpoint - feedback;
    pi->integ += pi->ki * error;
    if (pi->integ > pi->lim_hi) pi->integ = pi->lim_hi;
    if (pi->integ < pi->lim_lo) pi->integ = pi->lim_lo;
    float out = pi->kp * error + pi->integ;
    if (out > pi->lim_hi) out = pi->lim_hi;
    if (out < pi->lim_lo) out = pi->lim_lo;
    return out;
}
```

### 10.4 게인 추정 (초기값, 튜닝 필요)

배터리 충전 시스템은 시정수가 매우 느려서 게인 마진이 큼.

| 파라미터 | 초기값 |
|---|---|
| 충전 PI Kp | 0.05 |
| 충전 PI Ki | 1.0 (per-tick) |
| 방전 PI Kp | 0.05 |
| 방전 PI Ki | 1.0 |
| Integ limit | duty 0~1 |

실제 보드 동작 후 step response 보면서 튜닝.

### 10.5 CC ↔ CV 전환

```c
if (mode == CC) {
    error = (i_target - i_meas);
    if (vbat >= v_full) {
        mode = CV;
        pi.integ = previous_duty;  /* bumpless transfer */
    }
} else { /* CV */
    error = (v_full - vbat);
    if (i_meas < i_min_cv_exit) {  /* 0.05C 등 */
        mode = DONE;
    }
}
```

---

## 11. 안전 메커니즘 (필수)

| 계층 | 방식 | 응답 시간 |
|---|---|---|
| **HW OCP** | 션트 비교기 → SR 래치 → AND 게이트 → PWM gate | < 1 µs |
| **SW OCP** | 매 ADC 샘플 (85 kHz) 검사 | ~12 µs |
| **컨트롤 OCP** | 매 컨트롤 tick (8.5 kHz) 검사 | ~118 µs |
| **OVP** | VBAT > 한계 시 PWM 차단 | 컨트롤 tick |
| **UVP** | VBAT < 한계 시 방전 PWM 차단 | 컨트롤 tick |
| **OTP (배터리)** | BQ34Z100에서 I²C로 셀 온도 폴링 → 한계 시 차단 | 100 ms (UI tick) |
| **Soft-start** | duty 0에서 ramp | 100 ms |
| **Watchdog** | RP2350 watchdog timer 활용 | 1 s |
| **Anti-windup** | PI 적분기 saturation | 매 tick |

### HW OCP 회로 (확정 토폴로지)

**SR 래치 + AND 게이트** 방식:

```
션트 (5 mΩ) ──┐
              │
              ▼
        ┌─ LM393 ─┐
        │ 비교기  │ ──► HIGH=정상, LOW=과전류
        └─────────┘          │
                             ▼
                    ┌─ 74HC279 ─┐
        MCU RESET ──► R         │
                    │ SR latch  │ ──► Q (래치된 fault)
                    │           │
        FAULT in ──► S          │
                    └───────────┘
                          │
                          ▼ NOT_FAULT
                                                  ┌─────────┐
            PWM (RP2350 GP0/GP1) ────────────────►│   AND   │
                                                  │ 74HC08  │ ──► gate driver IN
                                                  └─────────┘
```

**동작**:
1. 정상 시: 비교기 HIGH → 래치 Q=HIGH (NOT_FAULT 활성) → AND 게이트 통과 → PWM이 게이트 드라이버에 도달
2. 과전류 시: 비교기 LOW → 래치 Q=LOW (NOT_FAULT 비활성) → AND 차단 → 게이트 드라이버 OFF → FET 즉시 차단
3. 래치 클리어 전까지 PWM 차단 유지 (반복 트립 방지)
4. MCU가 fault 원인 확인 후 RESET 신호로 래치 클리어

**부품**:
- LM393 (듀얼 비교기, 충전+방전 각 1채널)
- 74HC279 (Quad SR latch, 충전+방전 각 1)
- 74HC08 (Quad AND, PWM 2채널 처리)

NCP51313ADR2G에 EN/SD 핀이 있다면 AND 게이트 생략하고 SR 래치 출력을 직접 EN 핀에 연결하는 것도 가능 (단순화).

---

## 12. 통신 인터페이스

### 12.1 SPI0 (LCD, 이미 사용)
- ST75256 (JLX256160G-680)
- 8 MHz
- 핀: GP18 SCK, GP19 MOSI, GP17 CS, GP22 DC, GP16 RST

### 12.2 SPI1 (AD7606, 신규)
- 16 MHz
- 핀: GP10 SCK, GP11 MOSI, GP12 MISO, GP13 CS
- 추가: GP14 CONVST, GP15 BUSY (또는 인터럽트)
- DMA 채널 1개 (RX)

### 12.3 I²C0 (BQ34Z100, 신규)
- 100 또는 400 kHz (SMBus)
- 핀: GP4 SDA, GP5 SCL
- 외부 풀업 4.7 kΩ
- BQ34Z100과 별도 보드, 케이블 연결

### 12.4 I²C1 (EEPROM, 신규)
- 100 또는 400 kHz
- 핀: GP6 SDA, GP7 SCL
- 외부 풀업 4.7 kΩ
- **24LC256 32 KB EEPROM** (보드 로컬, 짧은 트레이스)
- BQ34Z100 케이블과 분리 → 노이즈/SI 영향 없음

### 12.5 USB (디버그)
- USB CDC stdio (Pico 표준)
- 디버그 로그, 명령 입력
- Core 1의 `log_task`에서만 사용 (Core 0는 절대 안 씀)

### 12.5b UART1 (외부 통신, 신규)
- 핀: **GP8 TX, GP9 RX**
- baud rate: TBD (사용자 협의 — 일반적으로 115200 bps)
- 프로토콜: TBD (RS-232 / RS-485 / 단순 UART)
- Core 1의 `uart_task`에서 처리 (FreeRTOS queue로 송수신 분리)
- DMA 사용 권장 (RX 연속, TX on-demand)

**용도 미정** — 다음 중 어느 것?
- (a) PC와 명령/모니터링 (USB CDC 외 추가 채널)
- (b) 외부 마스터 컨트롤러와 연결
- (c) RS-485로 다른 BMS/계측기와 데이지 체인
- (d) GPS / GSM / Wi-Fi 모듈 등 부가 모듈

용도가 정해지면 프로토콜 설계 추가.

### 12.6 설정 저장 (EEPROM)

사용자 설정값과 캘리브레이션 데이터를 비휘발성으로 보관.

| 항목 | 값 |
|---|---|
| 부품 | **Microchip 24LC256-I/SN** (또는 AT24C256C-SSHL-T) |
| 용량 | 32 KB (256 Kbit) |
| 인터페이스 | I²C, 100/400 kHz |
| Page size | 64 byte |
| Write cycle | ~5 ms / page |
| 내구성 | 1M write cycles / byte |
| 패키지 | SOIC-8 |
| 가격 | ~$0.80 |
| 어드레스 | 0x50 (A0/A1/A2 = GND) |

#### 저장 데이터

```c
typedef struct {
    uint32_t magic;           /* 0xCA170001 식별자 + 버전 */
    uint16_t version;         /* 구조 변경 추적 */

    /* SETTING 페이지 항목 */
    uint8_t  ich;             /* charge current step (×0.5 A) */
    uint8_t  idh;             /* discharge current step (×0.5 A) */
    uint16_t vfl;             /* full voltage ×100 V */
    uint16_t vcu;             /* cutoff voltage ×100 V */
    uint8_t  cycl;            /* learn cycles */
    uint8_t  rest;            /* rest hours ×0.5 */
    uint8_t  cells;           /* cells - 1 */

    /* Hall 센서 0 A offset (부팅 캘리브레이션) */
    float    hall_zero_iin;
    float    hall_zero_ichg;
    float    hall_zero_idchg;

    /* 분압 보정 (factory cal) */
    float    vin_scale, vbat_scale, vload_scale;

    /* 누적 사이클 카운터 (선택) */
    uint32_t total_cycles;

    uint8_t  reserved[16];
    uint32_t crc32;           /* 무결성 검증 */
} settings_t;
```

64 byte 이내로 맞추면 EEPROM 1 page에 맞아 atomic write 가능. 32 KB 중 첫 page만 사용하고 나머지는 추후 확장용 (학습 사이클 로그, RA 테이블 백업 등).

#### 멀티 코어 제약 없음

EEPROM은 외부 칩이라 플래시처럼 XIP 중단 문제가 없습니다. **Core 0 컨트롤 루프 실행 중에도 Core 1이 EEPROM 쓰기 가능**. 5 ms 동안 I²C가 점유되는 것 외엔 영향 없음.

#### 부팅/저장 흐름

```
부팅:
    settings_init()
        → EEPROM에서 64 byte read
        → magic + crc 검증
        → 유효: RAM 사본에 로드
        → 무효: 기본값으로 초기화 후 즉시 저장

런타임:
    settings_modify() → RAM만 변경
    settings_save()   → EEPROM에 commit (사용자 SAVE 액션 시)
```

#### EEPROM 외 보너스 활용

32 KB 중 첫 64 byte만 settings에 쓰고, 나머지 31.9 KB는:
- **학습 사이클 결과 로그**: 사이클별 QMAX, 적분 mAh, 시작/종료 시각 (1 사이클 ≈ 32 byte → 1000 사이클 가능)
- **OCV 곡선 백업**: BQ34Z100에서 읽은 OCV/RA 테이블 사본
- **펌웨어 버전 + 빌드 시간 이력**

추후 확장 시 page-aligned offset으로 영역 분할.

---

## 13. 핀 할당 (제안)

| GPIO | 용도 | 비고 |
|---|---|---|
| GP0 | 충전 PWM (PWM0A) | 85 kHz |
| GP1 | 방전 PWM (PWM0B) | 85 kHz, 동일 슬라이스 또는 별개 |
| GP2 | 충전 enable (gate driver EN) | 안전 차단용 |
| GP3 | 방전 enable | 안전 차단용 |
| GP4 | I²C0 SDA | BQ34Z100 (외부 보드, 케이블) |
| GP5 | I²C0 SCL | BQ34Z100 |
| GP6 | I²C1 SDA | **24LC256 EEPROM (로컬)** |
| GP7 | I²C1 SCL | EEPROM |
| GP8 | **UART1 TX** | 외부 통신 |
| GP9 | **UART1 RX** | 외부 통신 |
| GP10 | SPI1 SCK | AD7606 |
| GP11 | SPI1 TX | AD7606 |
| GP12 | SPI1 RX | AD7606 |
| GP13 | SPI1 CS | AD7606 |
| GP14 | AD7606 CONVST | PWM 동기 트리거 |
| GP15 | AD7606 BUSY | 변환 완료 |
| GP16 | LCD RST | 사용 중 |
| GP17 | LCD CS | 사용 중 |
| GP18 | LCD SCK (SPI0) | 사용 중 |
| GP19 | LCD MOSI (SPI0) | 사용 중 |
| GP20 | 버튼 DOWN | |
| GP21 | 버튼 LEFT | |
| GP22 | LCD DC (RS) | 사용 중 |
| GP23 | 버튼 RIGHT | |
| GP24 | OCP fault input | SR 래치 Q 모니터 |
| GP26 | OCP latch RESET | 래치 클리어 (active high pulse) |
| GP25 | LED (Pico 내장) | 사용 중 |
| GP27 | **버튼 TAB** | 풀업 (이동: GP8 → GP27) |
| GP28 | **버튼 UP** | 풀업 (이동: GP9 → GP28) |
| GP29 | VSYS 측정 (Pico 내장) | |

핀 부족하면 PCB에서 일부 재배치 가능. 25 / 26 GPIO 사용.

---

## 14. 소프트웨어 구조

### 14.1 디렉터리 (현재 + 추가 예정)

```
src/
├── app/
│   └── main.c                ✅ 데모 루프
├── lcd/                      ✅ ST75256 4-gray driver
│   ├── lcd.h
│   ├── lcd_st75256.c
│   └── lcd_st75256_cmds.h
├── ui/                       ✅ 7 페이지 UI
│   ├── font5x7.[ch]
│   ├── draw.[ch]
│   ├── sim.[ch]              (실측 데이터로 교체 예정)
│   └── ui.[ch]
├── hal/                      ✅ + 확장 예정
│   ├── hal_spi.h             ✅
│   ├── hal_gpio.h            ✅
│   ├── hal_delay.h           ✅
│   ├── hal_pwm.h             ✅
│   ├── hal_adc.h             ⏳ 추가
│   ├── hal_i2c.h             ⏳ 추가
│   └── hal_uart.h            ⏳ 추가
├── adc/                      ⏳ AD7606 드라이버
│   ├── ad7606.[ch]
│   └── adc_filter.[ch]       (오버샘플 평균)
├── pwm/                      ✅ 벅 PWM 추상화
│   └── buck_pwm.[ch]         85 kHz 2채널, set_duty / emergency_stop
├── control/                  ⏳ PI + CC/CV 머신
│   ├── pi.[ch]
│   └── buck_ctrl.[ch]
├── learn/                    ⏳ 학습 사이클 상태 머신
│   └── learn_cycle.[ch]
├── bms/                      ⏳ BQ34Z100 I²C 드라이버
│   └── bq34z100.[ch]
├── eeprom/                   ⏳ 24LC256 드라이버
│   └── eeprom_24lc256.[ch]
├── settings/                 ⏳ 설정 영구 저장 (EEPROM 위에 구축)
│   ├── settings.[ch]
│   └── settings_default.h
├── safety/                   ⏳ OCP/OVP/OTP/watchdog
│   └── safety.[ch]
├── uart_link/                ⏳ UART1 외부 통신 프로토콜
│   ├── uart_link.[ch]
│   └── uart_proto.[ch]
├── freertos_config/          ✅ FreeRTOS 설정
│   └── FreeRTOSConfig.h      single-core (Core 1 only), 1 ms tick, 32 KB heap
├── tasks/                    ✅ Core 1 FreeRTOS tasks
│   ├── core1_entry.[ch]      ✅ LCD init + task create + scheduler start
│   ├── lcd_task.[ch]         ✅ 10 Hz 렌더
│   ├── pwm_task.[ch]         ✅ buck PWM 초기화 + duty sweep 데모
│   ├── bms_task.c            ⏳ Phase 7b
│   ├── input_task.c          ⏳ Phase 10
│   ├── learn_task.c          ⏳ Phase 8
│   ├── uart_task.c           ⏳ Phase 9b
│   └── log_task.c            ⏳ 필요 시
└── port/
    ├── pico/                 ✅ Pico HAL 구현
    │   ├── board_config.h    ✅ LCD 핀 + BUCK PWM 핀/주파수
    │   ├── hal_spi_pico.c    ✅
    │   ├── hal_gpio_pico.c   ✅
    │   ├── hal_delay_pico.c  ✅
    │   ├── hal_pwm_pico.c    ✅
    │   ├── hal_adc_pico.c    ⏳
    │   ├── hal_i2c_pico.c    ⏳
    │   └── hal_uart_pico.c   ⏳
    └── mkv31/                ⏳ 향후 이식
```

### 14.2 모듈 의존성

```
            main.c (Core 0 + Core 1 entry)
              │
       ┌──────┴──────┐
       │             │
       ▼             ▼
   [Core 0 RT]   [Core 1 UI]
       │             │
       ▼             ▼
  control/         learn/
       │             │
       ▼             ▼
     pi/          bms/ ──► hal_i2c
       │             │
       ▼             ▼
    safety/       ui/ ──► draw/font5x7 ──► lcd/
       │             │
       ▼             │
   adc/             │
       │             │
       ▼             │
   pwm/             │
       │             │
       └──────┬──────┘
              ▼
            hal/
              │
              ▼
         port/<mcu>/
```

### 14.3 코어 데이터 교환

```c
/* shared.h */
typedef struct {
    /* 측정 */
    float vin, iin;
    float vbat, ibat;
    float i_chg, i_dchg;
    float vload;
    /* 누적 */
    uint32_t cumulative_mAh_chg;
    uint32_t cumulative_mAh_dchg;
    /* 상태 */
    uint8_t  ctrl_mode;     /* IDLE/CC/CV/REST/FAULT */
    uint8_t  fault_flags;
    uint32_t timestamp_ms;
    /* BQ에서 받은 (Core 1이 채움) */
    uint8_t  bq_soc, bq_soh;
    uint16_t bq_qmax;
} measurement_snapshot_t;

/* 더블 버퍼 */
extern measurement_snapshot_t snap_buf[2];
extern volatile uint32_t      snap_idx;

/* 명령 */
typedef enum {
    CMD_NONE,
    CMD_SET_MODE,    /* value = mode */
    CMD_SET_I_CHG,   /* value = A */
    CMD_SET_V_FULL,  /* value = V */
    CMD_SET_I_DCHG,
    CMD_START_LEARN,
    CMD_STOP,
} cmd_type_t;

typedef struct {
    cmd_type_t type;
    float      value;
} command_t;
```

---

## 15. BOM (Bill of Materials) 초안

### 15.1 컨트롤 / 측정

| 카테고리 | 부품 | 수량 | 용도 | 비고 |
|---|---|---|---|---|
| **MCU** | **Raspberry Pi Pico 2 (RP2350)** | 1 | 메인 컨트롤러 | ✅ 확정 |
| LCD | JLX256160G-680 (ST75256) | 1 | 256×160 4-gray | 이미 보유 |
| ADC | AD7606BSTZ | 1 | 8 ch 16-bit simultaneous | |
| **전류 센서** | **ACS725LLCTR-10AU-S** | **3** | IIN, I_CHG, I_DCHG | **5 V 공급, 0.5 V offset** |
| **EEPROM** | **24LC256-I/SN** | **1** | 설정 + 캘리브레이션 저장 | I²C1, 32 KB, 1M cycles |
| 정밀 분압 저항 | 0.1% 박막 | 8쌍 | VIN, VBAT, VLOAD 분압 + 예비 | 매칭 페어 |
| 노이즈 필터 | 1 nF C0G + 100 Ω | 6 | ADC 입력 RC LPF | |
| I²C 풀업 저항 | 4.7 kΩ × 4 | 1 | I²C0 + I²C1 SDA/SCL 풀업 | |

### 15.2 충전 벅 (×1)

| 부품 | 사양 | 후보 |
|---|---|---|
| HS NMOS | Vds ≥ 80 V, Rds(on) ≤ 30 mΩ, Qg 낮음 | BSC059N08NS5, IPB180N10S4 |
| **게이트 드라이버** | **NCP51313ADR2G** | ON Semi, 부트스트랩 내장 |
| 부트스트랩 캡 | 0.1 µF 16 V | C0G/X7R |
| Schottky 다이오드 | Vrrm ≥ 80 V, If ≥ 5 A | SBR10U200P5, MBRB1080 |
| 인덕터 | 100 µH, Isat ≥ 7 A, DCR ≤ 30 mΩ | Würth 7447789100, Coilcraft XAL |
| 입력 캡 | 470 µF 100 V 알루미늄 + 10 µF × 2 MLCC | UCC, Nichicon |
| 출력 캡 | 470 µF 50 V + 10 µF × 2 MLCC | |
| 션트 (HW OCP용) | 5 mΩ 1 W | Vishay WSL2512 |
| OCP 비교기 | **LM393** | 듀얼 (충전+방전 1칩으로) |
| OCP SR 래치 | **74HC279** | Quad SR latch |
| OCP AND 게이트 | **74HC08** | PWM 2채널 처리 |

### 15.3 방전 벅 (×1)

같은 구성. 입력 전압이 다르므로 캐패시터 정격 50 V로 다운 가능. 게이트 드라이버 동일 (**NCP51313ADR2G**).

### 15.4 dump load (확정)

| 부품 | 사양 | 수량 |
|---|---|---|
| **파워 저항** | **10 Ω, 10 W, 알루미늄 클래드** | **4 (병렬)** |
| → 합성 | **2.5 Ω, 40 W** | |
| 방열판 | 알루미늄 클래드 저항 자체 또는 보조 방열판 | |

후보 부품: TE Connectivity THS10R0J, Arcol HS10, Vishay RH010 시리즈 (모두 10 Ω 10 W 알루미늄 클래드 방열판형).

### 15.5 전원 / 보조

| 부품 | 사양 | 용도 |
|---|---|---|
| 48 V 입력 | DC 잭 또는 단자대 | 외부 어댑터 |
| 48 V → 12 V 컨버터 | TPS54360 등 | 게이트 드라이버 공급 |
| **12 V → 5 V LDO 또는 buck** | **AP2112K-5.0** 또는 **MCP1700-5002** | **Hall 센서 ACS725 공급 (~30 mA)** |
| 5 V → 3.3 V LDO | AP2112K-3.3 등 | RP2040 / ADC 공급 |
| Schottky 입력 보호 | 60 V 5 A | 역접속 방지 |
| TVS | SMBJ58A | 48 V 입력 surge 보호 |
| 퓨즈 | 5 A fast | 입력 보호 |

**전원 트리 (수정)**:
```
48 V (입력)
  ├─→ 충전 벅 입력
  ├─→ 방전 벅 입력은 VBAT (별개 경로)
  └─→ 48V→12V buck (TPS54360 등)
        ├─→ 게이트 드라이버 NCP51313 공급 (12 V)
        └─→ 12V→5V LDO
              ├─→ Hall 센서 ACS725 × 3 (5 V)
              └─→ 5V→3.3V LDO
                    ├─→ RP2040 / Pico
                    ├─→ AD7606 디지털
                    └─→ AD7606 아날로그 (또는 5 V 직접)
```

### 15.6 외부 보드 (이 PCB 외부)

| 부품 | 비고 |
|---|---|
| BQ34Z100 + 배터리 보드 | 별도 PCB, **셀 NTC 포함**, BQ가 셀 온도 측정 |
| I²C 케이블 | 4-pin (SDA, SCL, VCC, GND) |
| 배터리 팩 | 3S~6S Li-ion, BQ 보드와 함께 |

**이 PCB의 역할**: 충방전 컨트롤러 (PWM, ADC, 벅, 컨트롤 루프, UI)
**외부 보드의 역할**: BMS (BQ34Z100 + 배터리 셀 + 셀 NTC + 보호 회로)
**연결**: I²C 케이블 1개 (전원/SDA/SCL/GND)

---

## 16. 미결정 사항

| # | 항목 | 상태 |
|---|---|---|
| 1 | ~~MCU 업그레이드~~ | ✅ **확정: Raspberry Pi Pico 2 (RP2350)** |
| 2 | ~~버튼 5개~~ | ✅ **확정: GP8/9/20/21/23 (TAB/UP/DOWN/LEFT/RIGHT)** |
| 3 | ~~dump load 값~~ | ✅ **확정: 10 Ω 10 W × 4 병렬 = 2.5 Ω 40 W** |
| 4 | ~~셀 구성 범위~~ | ✅ **확정: 3S~6S Li-ion (12.6~25.2 V)** |
| 5 | ~~충전기 입력~~ | ✅ **확정: 48 V DC 고정** |
| 6 | ~~OCP 회로 토폴로지~~ | ✅ **확정: SR 래치(74HC279) + AND(74HC08), LM393 비교기** |
| 7 | ~~NTC~~ | ✅ **확정: 이 PCB에는 없음**. 셀 온도는 BQ34Z100 보드에서 I²C로 받음 |
| 8 | ~~PCB 통합 vs 분리~~ | ✅ **확정: 충방전 컨트롤러만 본 PCB, BMS는 별도 보드** |
| 9 | ~~BQ34Z100 사전 설정~~ | ✅ **확정: bqStudio로 외부 사전 등록**. 펌웨어는 학습 사이클만 |
| 10 | ~~NCP51313ADR2G 토폴로지~~ | ✅ **확정: HS only → 비동기 buck** |
| 11 | ~~Hall 센서~~ | ✅ **확정: ACS725LLCTR-10AU-S** |
| 12 | ~~설정 저장~~ | ✅ **확정: 24LC256 EEPROM, I²C1** |
| 13 | ~~듀얼 코어 분담~~ | ✅ **확정: Core 0 bare-metal, Core 1 FreeRTOS** |
| 14 | ~~UART 추가~~ | ✅ **핀 확정: GP8/GP9 (UART1)**, **용도/프로토콜은 미정** |
| 15 | UART 용도 / 프로토콜 / baud | PC통신? RS-485? 외부 마스터? |

**모든 주요 미결정 사항 해결됨.** 다음 단계는 펌웨어 골격(HAL 확장 + 모듈 stub) 또는 PCB 회로 진행.

---

## 17. 개발 단계 로드맵

### Phase 1 — LCD/UI ✅ 완료
- [x] LCD 드라이버 (4-gray ST75256)
- [x] 5×7 폰트 + 그리기 헬퍼
- [x] 7 페이지 UI 데모 (시뮬 데이터)
- [x] 10 Hz 갱신 + 프레임 카운터

### Phase 2 — MCU 업그레이드 (확정)
- [ ] SDK 2.x 설치 (VS Code Pico 확장 권장)
- [ ] CMakeLists 수정 (`PICO_BOARD pico2`, `PICO_PLATFORM rp2350`)
- [ ] Pico 2 보드 교체 + 빌드 검증
- [ ] LCD/UI 동작 확인 (변경 없이 동작해야 함)

### Phase 3 — HAL 확장
- [ ] `hal_pwm.h` + `hal_pwm_pico.c` (85 kHz, 2 슬라이스)
- [ ] `hal_adc.h` + `hal_adc_pico.c` (AD7606 SPI + DMA)
- [ ] `hal_i2c.h` + `hal_i2c_pico.c` (I²C0, BQ34Z100용)
- [ ] HAL 단위 테스트 (loopback, 더미)

### Phase 4 — ADC / PWM 모듈
- [x] **PWM 85 kHz 두 슬라이스 셋업** (GP0 slice 0A, GP1 slice 0B)
- [x] **hal_pwm + buck_pwm 모듈** (core-agnostic)
- [x] **pwm_task** (Core 1 FreeRTOS, duty sweep 데모)
- [ ] AD7606 드라이버 (SPI, DMA, CONVST 트리거)
- [ ] 6채널 ring buffer + 평균 함수
- [ ] 측정 채널 캘리브레이션 (gain/offset)

### Phase 5 — 컨트롤 루프 + FreeRTOS 도입
- [x] **FreeRTOS-Kernel 통합** (Raspberry Pi fork, FetchContent 자동)
- [x] **`FreeRTOSConfig.h`** (Core 1 only, single-core mode, 1 ms tick, 32 KB heap)
- [x] **Core 1 부트 → FreeRTOS scheduler 시작** (`core1_entry.c`)
- [x] **첫 task: `lcd_task`** (기존 데모 루프 → task로 이전, `vTaskDelayUntil`로 정확한 10 Hz)
- [x] **두 번째 task: `pwm_task`** (priority 4, buck PWM 초기화 + duty sweep)
- [ ] PI 컨트롤러 (float, anti-windup, bumpless)
- [ ] CC/CV 모드 머신
- [ ] 8.5 kHz 타이머 + tick handler (Core 0 bare-metal OR Core 1 HW timer ISR)
- [ ] 듀얼 코어 데이터 교환 인프라 (스냅샷 더블 버퍼, 명령 큐)
- [ ] Soft-start

### Phase 6 — 안전
- [ ] HW OCP (보드 회로)
- [ ] SW OCP/OVP/OTP 코드
- [ ] Watchdog
- [ ] Fault state 및 복구

### Phase 7 — BMS + EEPROM 통합
- [ ] 24LC256 EEPROM I²C 드라이버 (`src/eeprom/`)
- [ ] settings 모듈 (load/save/CRC, 기본값) (`src/settings/`)
- [ ] 부팅 시 자동 로드 + 무효 시 기본값 저장
- [ ] Hall 캘리브레이션 (0 A offset) 부팅 시 측정 + EEPROM 저장
- [ ] BQ34Z100 I²C 드라이버
- [ ] BQ 명령 wrapping (Read SOC/SOH/QMAX)
- [ ] 학습 cycle 명령 (`IT_ENABLE`, `LEARN_GO` 등)
- [ ] BQ 상태 폴링 → UI 표시

### Phase 8 — 학습 사이클 머신
- [ ] 상태 머신 (IDLE → CC → CV → REST → CC discharge → REST → 반복)
- [ ] 사이클 횟수 카운터
- [ ] 사이클 종료 조건 / 에러 처리
- [ ] BQ 학습 진행률 반영

### Phase 9 — UI 실데이터 연결
- [ ] 시뮬 SS 배열 → 실측 스냅샷으로 교체
- [ ] BQ 데이터 (SOC/SOH/QMAX) → BATTERY 페이지
- [ ] 학습 진행 단계 → LEARN 페이지
- [ ] 사용자 입력 (CONTROL/SETTING 페이지)

### Phase 9b — UART 통신 (병렬 가능)
- [ ] UART1 HAL (`hal_uart.h` + `hal_uart_pico.c`)
- [ ] uart_task: TX queue + RX queue + DMA
- [ ] 프로토콜 정의 (사용자 협의 후)
- [ ] 명령 처리 / 응답 / 모니터링 패킷

### Phase 10 — 버튼 입력 (선택)
- [ ] `hal_input.h` + GPIO 폴링/디바운스
- [ ] UI 입력 디스패처
- [ ] 데모 자동 순환 → 실제 입력 모드 전환

### Phase 11 — MKV31 이식 (먼 미래)
- [ ] `port/mkv31/` 디렉터리 채우기
- [ ] KSDK 또는 베어메탈 SPI/I²C/PWM/ADC
- [ ] CMakeLists / linker script
- [ ] 동작 검증

---

## 18. 참고 자료

### 데이터시트
- ST75256: STMicroelectronics ST75256 256×160 LCD controller
- AD7606: Analog Devices 8-Channel 16-Bit DAS Simultaneous Sampling
- ACS70331: Allegro 1 MHz Hall-Effect Linear Current Sensor IC
- BQ34Z100: Texas Instruments Multi-Cell Impedance Track Fuel Gauge
- RP2040 / RP2350: Raspberry Pi Pico SDK Documentation

### 설계 참고
- TI Application Report: "Designing With the BQ34Z100"
- Erickson & Maksimović, "Fundamentals of Power Electronics" — buck topology
- Robert Sheehan, "Switch-Mode Power Converter Compensation Made Easy" — PI 튜닝

---

## 19. 변경 이력

| 날짜 | 변경 |
|---|---|
| 2026-04-10 | 초안. LCD/UI Phase 1 완료. ADC/벅 설계 결정 정리. |
| 2026-04-10 | 부품 확정: Hall = ACS725LLCTR-10AU-S, 게이트 드라이버 = NCP51313ADR2G, dump load = 10Ω 10W × 4 병렬 (2.5Ω 40W). 5V rail 추가, 1S 방전 한계 1.68A 명시. |
| 2026-04-10 | 셀 범위 확정: 3S~6S (12.6~25.2 V). NCP51313 = HS only → 비동기 buck 확정. 인덕터 6S 기준 100 µH 재계산. |
| 2026-04-10 | 24LC256 EEPROM 추가 (I²C1, 32 KB). 설정/캘리브레이션 영구 저장. settings 모듈 + eeprom 드라이버 추가 예정. |
| 2026-04-10 | 모든 미결정 사항 해결: MCU=Pico 2 확정, 버튼 GPIO 할당, 입력 48V 고정, OCP 회로(SR latch + AND), NTC 없음(BQ가 처리), PCB 분리 확정, BQ chemistry는 bqStudio로 외부 등록. |
| 2026-04-11 | FreeRTOS 도입 결정: Core 0 bare-metal, Core 1 FreeRTOS (single-core scheduler). UART1 추가 (GP8/GP9). 버튼 TAB/UP을 GP27/GP28로 이동. UART 용도/프로토콜은 미정. |
| 2026-04-11 | FreeRTOS-Kernel 통합 완료 (Raspberry Pi fork, FetchContent). Core 1 FreeRTOS scheduler 동작. lcd_task가 기존 데모 루프를 대체. 바이너리 50 KB → 100 KB. |
| 2026-04-11 | PWM 제어 계층 구현: hal_pwm + buck_pwm 모듈 + pwm_task. GP0/GP1이 85 kHz slice 0 A/B에서 듀티 sweep. board_config.h에 BUCK_*_PWM_PIN, BUCK_PWM_FREQ_HZ 추가. |
