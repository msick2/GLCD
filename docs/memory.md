# 프로젝트 메모리 (세션 인계용)

마지막 업데이트: 2026-04-11 (**프로젝트 종료 — MKV31F512로 이전 결정**)
GitHub: https://github.com/msick2/GLCD (Pico 레퍼런스 아카이브)

> **이 repo는 여기서 "완성" 상태로 동결됩니다.**
> 이후 개발은 **새 프로젝트 / 새 세션**에서 **MKV31F512** 타겟으로 진행합니다.
> 새 세션 핸드오프는 [mkv31_migration.md](mkv31_migration.md) 전체 문서를 참조하세요.

이 문서는 다른 세션에서 작업을 이어받을 수 있도록 정리한 핸드오프 노트입니다.
설계의 "왜"는 [design_guide.md](design_guide.md)에 있고, 이 문서는 "어디까지 했고, 다음에 뭐 할지"에 집중합니다.

---

## 1. 한 줄 요약

BQ34Z100 퓨얼게이지 학습용 정밀 충방전 보드. **Pico (RP2040/RP2350) 에서 LCD/UI/FreeRTOS/PWM까지 완성**. 그러나 **실 제품 MCU는 MKV31F512VLH12로 변경 확정** (LQFP-64, 120 MHz, M4+FPU, 내장 16-bit ADC × 2, FlexTimer). 이 Pico 프로젝트는 **레퍼런스 / 재사용 소스 아카이브**로 역할 전환. 다음 세션은 **새 프로젝트**에서 MKV31 기반으로 시작하고, `src/lcd/`, `src/ui/`, `src/pwm/`, `src/freertos_config/`, `src/tasks/` 등을 그대로 복사해서 재사용. 핸드오프 문서: [mkv31_migration.md](mkv31_migration.md).

---

## 1b. 새 세션 시작 가이드 (중요)

**새 세션을 여는 사람을 위한 메시지**:

1. 이 repo (`msick2/GLCD`)는 **Pico 기반 레퍼런스**입니다. 실제 제품은 MKV31F512로 만듭니다.
2. 반드시 먼저 읽을 문서: **[mkv31_migration.md](mkv31_migration.md)**
3. 그 문서에 Checkpoint 1~10 순서가 있습니다. 1번부터 차례로 진행하세요.
4. 이 repo의 `src/lcd/`, `src/ui/`, `src/pwm/`, `src/freertos_config/`, `src/tasks/`, `src/hal/hal_*.h`는 **그대로 복사**합니다. MCU-specific 코드는 `src/port/pico/` 안에만 있으므로 거기만 버리면 됩니다.
5. 새 프로젝트 폴더를 만들고 이 파일들을 복사한 뒤, `src/port/mkv31/`을 새로 작성하는 것이 핵심 작업입니다.
6. MCUXpresso IDE로 먼저 빈 프로젝트 + Config Tool로 클럭/핀 설정 → 그 결과물을 CMake + VS Code 환경으로 이전 → 이 repo의 재사용 소스 통합 → 차례로 checkpoint 달성.

---

## 2. 프로젝트 목표

- **하드웨어**: 48 V 입력 → 충전 벅 + 방전 벅 (두 비동기 buck) → 3S~6S Li-ion + dump load
- **목적**: BQ34Z100 BMS IC의 학습 사이클 (CC charge → CV → REST → CC discharge → REST 반복) 자동화
- **측정**: ADC 6채널 (VIN, IIN, VBAT, I_CHG, I_DCHG, VLOAD)
- **UI**: 256×160 4-gray LCD에 7개 페이지 표시
- **이식 목표**: 향후 NXP MKV31 (Cortex-M4F)로 옮길 수 있도록 HAL 분리 구조

---

## 3. 현재 상태

### ✅ Phase 1 — LCD/UI 완료
- ST75256 native 4-gray 모드 동작 (256×160, MSB-top, 4-row-per-page 메모리 구조)
- 5×7 ASCII 폰트 (47 글자)
- 그리기 헬퍼 (hline/vline/box/text/text_right)
- 7 페이지 UI 렌더러 (SUMMARY, CHARGE, DISCHARGE, BATTERY, LEARN, CONTROL, SETTING)
- 시뮬 데이터 6개 상태 자동 순환 데모 (1.5초마다 sim_idx, 9초마다 page 전환)
- 10 Hz 갱신 (100 ms 주기, 8 MHz SPI flush ~10 ms)
- 헤더 우측에 프레임 카운터 (10 Hz 검증용)
- Vop = 0x24 / 0x04 (튜닝 완료)
- SPI 클럭 8 MHz

### ✅ Phase 5 부분 — FreeRTOS Core 1 포팅 완료
- Raspberry Pi FreeRTOS-Kernel fork를 FetchContent로 자동 가져옴
- `src/freertos_config/FreeRTOSConfig.h` (single-core, 1 ms tick, 32 KB heap)
- `src/tasks/core1_entry.c`: LCD 초기화 + 태스크 생성 + `vTaskStartScheduler()`
- `src/tasks/lcd_task.c`: 데모 루프를 FreeRTOS task로 이식, `vTaskDelayUntil`로 정확한 10 Hz
- `src/app/main.c`: Core 0은 stdio만 init 후 `multicore_launch_core1()` → `__wfi()` 루프
- 바이너리 크기: 50 KB → 100 KB (FreeRTOS 커널 ~50 KB 추가)
- `configNUMBER_OF_CORES = 1` — RTOS는 Core 1 전용, Core 0는 bare-metal 유지 (컨트롤 루프 추가 준비)

### ✅ Phase 2 — SDK 2.1.1 + Pico 1/2 듀얼 타겟 완료
- Pico SDK **2.1.1** clone at `E:/pico-sdk-2/sdk` (shallow, 366 MB with submodules)
- **picotool 2.2.0-a4** at `E:/pico-sdk-2/tools/picotool` (RP2350 UF2 family 0xe48bff59 지원)
- **기존 GCC 10.3 재사용** — M33 (Cortex-M33) 지원 확인됨, 새 툴체인 설치 불필요
- `pico-env-v2.cmd` — PICO_SDK_PATH를 2.1.1로 override + picotool 2.x를 PATH 앞에
- `build-v2.cmd [pico|pico2] [clean]` — 타겟별 별도 디렉터리 `build-v2-<board>/`
- `FreeRTOSConfig.h`에 M33 매크로 4개 추가: `configENABLE_FPU=1`, `configENABLE_MPU=0`, `configENABLE_TRUSTZONE=0`, `configRUN_FREERTOS_SECURE_ONLY=1` (RP2040에서는 무시됨)
- **빌드 결과**:
  - RP2040: [build-v2-pico/glcd.uf2](../build-v2-pico/glcd.uf2) — 102 KB
  - RP2350: [build-v2-pico2/glcd.uf2](../build-v2-pico2/glcd.uf2) — 96 KB (FPU hard-float, 변수로 정렬 최적화 덕분에 더 작음)
- 기존 `build.cmd` (SDK 1.5.1) + `build/` 는 그대로 남겨둠, 호환성용
- **소스 코드는 한 줄도 변경 없이 양쪽 모두 빌드됨** (FreeRTOSConfig 매크로 4줄 추가 제외)

### ✅ Phase 4 부분 — PWM 제어 계층 완료 (Core 1)
- `src/hal/hal_pwm.h` — 추상 PWM 인터페이스 (opaque channel, init/set_duty/enable)
- `src/port/pico/hal_pwm_pico.c` — RP2040 구현 (`hardware/pwm.h`, divider 1.0, top 계산)
- `src/pwm/buck_pwm.[ch]` — BUCK_CHARGE / BUCK_DISCHARGE 관리, duty 캐시, `buck_pwm_emergency_stop`
- `src/tasks/pwm_task.c` — Core 1 FreeRTOS task (priority 4), buck 초기화 + duty sweep 데모
  - CHARGE: 0% ↔ 50% sweep, 10 ms/1% step
  - DISCHARGE: 0% ↔ 80% sweep, 10 ms/2% step
- 핀: GP0 = slice 0 A (CHARGE), GP1 = slice 0 B (DISCHARGE) — 동일 슬라이스라 주파수 정확히 동기
- 주파수: `top+1 = 125M/85k = 1470.588` → top=1470, 실측 84.97 kHz (0.04% 오차)
- duty 분해능: 1/1471 ≈ 0.068%
- `board_config.h`에 `BUCK_CHARGE_PWM_PIN = 0`, `BUCK_DISCHARGE_PWM_PIN = 1`, `BUCK_PWM_FREQ_HZ = 85000` 추가

### ⏳ Phase 3, 4(ADC), 6~11 — 미착수
모든 설계 결정은 끝났음. 코딩만 남음.

---

## 4. 확정된 부품 (BOM)

| 카테고리 | 부품 | 비고 |
|---|---|---|
| **MCU (실 제품)** | **NXP MKV31F512VLH12** | LQFP-64, 120 MHz, M4+FPU, 4 MHz 크리스탈, USB 없음 |
| MCU (이 repo 아카이브) | ~~Raspberry Pi Pico 2 (RP2350)~~ | 레퍼런스만, 실 제품에는 미사용 |
| ADC | **MKV31 내장 16-bit × 2** (외장 없음) | AD7606 제거, FlexTimer 하드웨어 트리거 |
| ADC Vref | **REF3030AIDBZT** (3.0 V, ±0.2%) | VREFH 외부 레퍼런스 |
| 디버거 | **J-Link** (사용자 보유) | SWD, UART VCOM 디버그 |
| **LCD** | JLX256160G-680 (ST75256) | 256×160 4-gray, 동작 검증 완료 |
| **ADC** | AD7606BSTZ | 16-bit, 8 ch simultaneous, ±10 V |
| **전류 센서** | ACS725LLCTR-10AU-S × 3 | 0~10 A, 5 V 공급, 200 mV/A, 0.5 V offset |
| **EEPROM** | Microchip 24LC256-I/SN | 32 KB, I²C1 |
| **게이트 드라이버** | NCP51313ADR2G × 2 | HS only, 부트스트랩 내장 |
| **벅 토폴로지** | 비동기 (HS NMOS + Schottky) | LS FET 없음 |
| **PWM** | 85 kHz, 2 슬라이스 (충전/방전 독립) | |
| **컨트롤 루프** | 8.5 kHz (= PWM/10) | float OK on Pico 2 |
| **방전 부하** | 10 Ω 10 W × 4 병렬 = 2.5 Ω 40 W | 안전 한계 4 A |
| **OCP 회로** | LM393 + 74HC279 SR latch + 74HC08 AND | 하드웨어 즉시 차단 |
| **셀 범위** | 3S~6S Li-ion (12.6~25.2 V) | |
| **입력** | DC 48 V 고정 | |
| **NTC** | 본 PCB 없음 | 셀 온도는 BQ34Z100 보드(외부)에서 I²C로 받음 |

---

## 5. 핀 할당 (Pico 1/2 공통 GPIO 매핑)

| GPIO | 용도 | 상태 |
|---|---|---|
| GP0 | **충전 PWM (slice 0 A, 85 kHz)** | **사용 중** (pwm_task sweep) |
| GP1 | **방전 PWM (slice 0 B, 85 kHz)** | **사용 중** (pwm_task sweep) |
| GP2 | 충전 enable (gate driver) | 미사용 |
| GP3 | 방전 enable | 미사용 |
| GP4 | I²C0 SDA (BQ34Z100, 외부 보드) | 미사용 |
| GP5 | I²C0 SCL | 미사용 |
| GP6 | I²C1 SDA (24LC256 EEPROM, 로컬) | 미사용 |
| GP7 | I²C1 SCL | 미사용 |
| GP8 | **UART1 TX** (외부 통신) | 미사용 |
| GP9 | **UART1 RX** | 미사용 |
| GP10 | SPI1 SCK (AD7606) | 미사용 |
| GP11 | SPI1 TX | 미사용 |
| GP12 | SPI1 RX | 미사용 |
| GP13 | SPI1 CS | 미사용 |
| GP14 | AD7606 CONVST | 미사용 |
| GP15 | AD7606 BUSY | 미사용 |
| **GP16** | **LCD RST** | **사용 중** |
| **GP17** | **LCD CS** | **사용 중** |
| **GP18** | **LCD SCK (SPI0)** | **사용 중** |
| **GP19** | **LCD MOSI (SPI0)** | **사용 중** |
| GP20 | 버튼 DOWN | 미사용 |
| GP21 | 버튼 LEFT | 미사용 |
| **GP22** | **LCD DC (RS)** | **사용 중** |
| GP23 | 버튼 RIGHT | 미사용 |
| GP24 | OCP fault input | 미사용 |
| **GP25** | **LED (Pico 내장)** | **사용 중 (5 Hz 깜빡임)** |
| GP26 | OCP latch RESET | 미사용 |
| GP27 | **버튼 TAB** (이동) | 미사용 |
| GP28 | **버튼 UP** (이동) | 미사용 |
| GP29 | VSYS (Pico 내장) | |

---

## 6. 빌드 / 실행

### 환경
- Windows 10
- pico-setup-windows v1.5.1 설치 (`C:\Program Files\Raspberry Pi\Pico SDK v1.5.1`) — 툴체인 제공
- **Pico SDK 2.1.1** at `E:\pico-sdk-2\sdk` — shallow clone + 서브모듈
- **picotool 2.2.0** at `E:\pico-sdk-2\tools\picotool` — RP2350 UF2 family 지원
- GCC ARM 10.3 (SDK 1.5.1 설치에서 재사용), CMake 3.25, Ninja 1.11, Python 3.9
- VS Code + cmake-tools + cortex-debug

### 빌드 (권장: SDK 2.x)
```cmd
:: Pico 1 (RP2040) 증분 빌드
build-v2.cmd pico

:: Pico 2 (RP2350) 증분 빌드
build-v2.cmd pico2

:: 클린 빌드
build-v2.cmd pico clean
build-v2.cmd pico2 clean
```

`build-v2.cmd`는 내부적으로 `pico-env-v2.cmd`를 source해서 PICO_SDK_PATH를 SDK 2.1.1으로 override + picotool 2.x를 PATH 앞에 놓고 `cmake -G Ninja -DPICO_BOARD=<board> .. && ninja` 실행.

### 빌드 (레거시: SDK 1.5.1, 호환성용)
```cmd
build.cmd          :: 증분
build.cmd clean    :: 클린
```

`build.cmd`는 기존 SDK 1.5.1 환경으로 빌드. Pico 1만 지원.

### 출력
- **SDK 2.x 빌드**:
  - `build-v2-pico/glcd.uf2` (~102 KB, RP2040) — Pico 1 굽기
  - `build-v2-pico2/glcd.uf2` (~96 KB, RP2350) — Pico 2 굽기
- **SDK 1.5.1 빌드** (레거시): `build/glcd.uf2` (~100 KB, RP2040 only)

### Flash
1. Pico를 BOOTSEL 누른 채 USB 연결 → RPI-RP2 드라이브 마운트
2. `glcd.uf2`를 그 드라이브에 드래그
3. 자동 재부팅, LCD에 데모 표시 시작

또는 `picotool`로 굽기 (이미 설치됨, `pico-env.cmd` 환경에서):
```
picotool load -x build/glcd.uf2 -f
```

---

## 7. 코드 구조

```
src/
├── app/main.c                ← Core 0 부트: stdio init + launch Core 1 + __wfi 루프
├── lcd/                      ← ST75256 4-gray 드라이버 ✅
│   ├── lcd.h                 - 공용 API + LCD_COLOR_*
│   ├── lcd_st75256.c         - init, draw_pixel, fill, flush, test fns
│   └── lcd_st75256_cmds.h    - 명령 상수
├── ui/                       ← 7 페이지 UI 렌더러 ✅
│   ├── font5x7.[ch]          - 5x7 ASCII 비트맵 폰트
│   ├── draw.[ch]             - hline/vline/box/text 헬퍼
│   ├── sim.[ch]              - 시뮬 데이터 6개 상태
│   └── ui.[ch]               - 탭/헤더/7페이지 렌더러
├── hal/                      ← 추상 인터페이스 ✅ (확장 예정)
│   ├── hal_spi.h             ✅
│   ├── hal_gpio.h            ✅
│   ├── hal_delay.h           ✅
│   ├── hal_pwm.h             ✅  NEW: init / set_duty / enable
│   ├── hal_adc.h             ⏳ Phase 3
│   ├── hal_i2c.h             ⏳ Phase 3
│   └── hal_uart.h            ⏳ Phase 3
├── adc/                      ⏳ Phase 4 — AD7606 드라이버
├── pwm/                      ✅ 벅 PWM 추상화
│   └── buck_pwm.[ch]         BUCK_CHARGE / BUCK_DISCHARGE, set_duty, emergency_stop
├── control/                  ⏳ Phase 5 — PI + CC/CV 머신
├── safety/                   ⏳ Phase 6 — OCP/OVP/OTP/watchdog
├── eeprom/                   ⏳ Phase 7a — 24LC256 드라이버
├── settings/                 ⏳ Phase 7a — 설정 영구 저장
├── bms/                      ⏳ Phase 7b — BQ34Z100 I²C
├── learn/                    ⏳ Phase 8 — 학습 사이클 머신
├── freertos_config/          ✅ FreeRTOSConfig.h (single-core, Core 1)
├── tasks/                    ✅ Core 1 FreeRTOS tasks
│   ├── core1_entry.[ch]      ✅ LCD init + task create + vTaskStartScheduler
│   ├── lcd_task.[ch]         ✅ 10 Hz 렌더
│   └── pwm_task.[ch]         ✅ buck PWM init + duty sweep 데모
└── port/
    ├── pico/                 ✅ Pico HAL 구현
    │   ├── board_config.h    - LCD 핀, Vop, BUCK PWM 핀/주파수
    │   ├── hal_spi_pico.c
    │   ├── hal_gpio_pico.c
    │   ├── hal_delay_pico.c
    │   └── hal_pwm_pico.c    ✅ NEW
    └── mkv31/                ⏳ 향후 이식 (README만 있음)
```

`docs/`:
- `design_guide.md` — 설계 결정 + BOM + 핀 + 컨트롤 루프 + 안전 + 로드맵 (1000줄)
- `memory.md` — 이 문서 (세션 인계)

`.claude/agents/`:
- `commenter.md` — 주석 추가 전용 Haiku 에이전트 (논리 변경 금지)
- `build-analyzer.md` — 빌드 실행 + 에러 분석 Haiku 에이전트 (코드 수정 금지)

---

## 8. 핵심 발견 (LCD 브링업 중 학습한 것들)

이 노트는 ST75256/JLX256160G 모듈을 다시 만질 때 시간을 줄여줍니다.

### ST75256 chip-specific quirks
1. **MSB-top bit ordering**: 페이지 내에서 비트 7이 윗 행, 비트 0이 아래 행. SSD1306과 반대. `draw_pixel`에서 `1 << (7 - y%8)` 사용.
2. **4-gray 모드는 page = 4 rows**: mono 모드는 8 rows/page, 4-gray 모드는 4 rows/page. 같은 5120 byte를 mono로 보낼 때와 4-gray로 보낼 때 페이지 수가 다름. **이걸 모르면 화면 절반만 갱신되고 나머지 절반은 이전 데이터가 남음.**
3. **0x5C Write Data 명령 필수**: 페이지/컬럼 어드레스 설정 후 0x5C를 보내야 RAM에 데이터가 들어감. 생략하면 무시됨.
4. **CS는 cmd+data transaction 동안 low로 유지 권장**: 매 바이트마다 CS toggle해도 작동하지만 0x5C 같은 enter-mode 명령은 CS 유지가 안전.
5. **`lcd_set_contrast()`의 high byte는 init과 같아야 함**: 다르면 chip이 명령 무시함. 우리는 0x04로 통일.
6. **Vop 인코딩이 일반 STN과 반대**: 숫자 ↑ = 어두움 ↑이 아니라, **숫자 ↑ = 더 밝음 (?)**. 실측으로 0x24 / 0x04가 최적.
7. **canonical JLX256160G init 시퀀스 확인됨**:
   - EXT2에서 0xD7,0x9F (auto read disable) + 0x32,0x00,0x01,0x04 (analog set, bias 1/12)
   - 0x20 + 16 byte gray level table (mono에선 안 써도 되지만 4-gray에선 필요)
   - EXT0로 돌아가서 0x75 page addr, 0x15 col addr, 0xBC 0x00 0xA6 (scan dir, **2 byte 데이터**)
   - 0xCA 0x00 0x9F 0x20 (display ctrl)
   - 0xF0 0x11 (4-gray 모드, mono는 0x10)
   - 0x81 + Vop low + Vop high
   - 0x20 0x0B (power ctrl)
   - 100 ms 대기
   - 0xAF (display on)

### Pico 1 (RP2040) specific
- **하드웨어 FPU 없음**: float은 ROM 보조로 처리, mul ~20 cycle, div ~150 cycle
- **MSB-top 비트가 mono와 4-gray 모두 동일**: 통일된 매핑 사용 가능
- 4-gray pixel packing: 1 byte = 4 pixel × 2 bit, MSB가 윗 행
  ```c
  /* y=0 → bits[7:6], y=1 → [5:4], y=2 → [3:2], y=3 → [1:0] */
  shift = (3 - (y & 3)) * 2;
  ```

### Build / 환경
- pico-env.cmd가 PICO_INSTALL_PATH를 내부에서 설정하지 않으므로, 우리 로컬 wrapper가 환경변수를 먼저 설정한 후 source해야 함
- `cmd //c "build.cmd"` 형태로 git bash에서 호출 (절대 경로 + 더블 슬래시)
- CRLF line ending 경고는 무시 (Windows 정상)

### LCD 디버깅 history
순서대로:
1. 화면 안 나옴 → init 시퀀스 / Vop / SPI mode / clock 추측
2. 컨트라스트 변화 보임 → SPI 통신 OK
3. 0xFF/0x00 fill 차이 살짝 보임 → 데이터 RAM 도달 확인
4. 8픽셀 단위 상하 반전 → MSB-top vs LSB-top 발견
5. 4-gray 모드에서 격자 → bit-plane vs pixel-packed 잘못 알았음
6. 4-gray 모드에서 화면 절반만 → page = 4 rows임을 발견
7. 1,2단계 같은 색 → gray level table 값 분리
8. 컨트라스트 미세 조정 → Vop 0x24

각 단계마다 lcd_test_*() 함수 추가해서 진단. 지금은 전부 정상.

---

## 9. 컨트롤 루프 설계 (Phase 5 시작 전 참고)

```
타이머 8.5 kHz (Core 0 ISR)
    │
    ▼
control_tick():
    1. ADC ring buffer에서 최근 10 샘플 평균 (각 채널)
       (ADC는 PWM과 동기 85 kHz로 백그라운드 DMA)
    2. mode 결정 (CC vs CV)
    3. PI step (충전 벅 + 방전 벅 각각)
    4. 안전 체크 (OCP/OVP/OTP)
    5. PWM duty 갱신
    6. 측정 스냅샷 publish (Core 0 → Core 1, double buffer)
```

PI 게인 초기값: Kp=0.05, Ki=1.0 (per-tick), 실 보드에서 step response 보고 튜닝.

CC ↔ CV 전환:
- VBAT >= V_full → CC → CV (bumpless: pi.integ = previous_duty)
- I_meas < 0.05 C → CV → DONE

방전 전류 클램프:
```c
I_dchg_max = min(VBAT / 2.5, 4.0);  /* dump load 2.5 Ω, 안전 4 A */
```

---

## 10. 듀얼 코어 분담 (현재 상태 + Phase 5 목표)

**Core 0 = bare-metal**, **Core 1 = FreeRTOS (single-core scheduler)**

### 현재 (Phase 5 일부 구현)
```
Core 0 (bare-metal)             Core 1 (FreeRTOS)
└─ stdio + multicore_launch      ├─ lcd_task    ✅ (10 Hz 렌더)
   └─ while(1) __wfi()            └─ pwm_task    ✅ (buck init + duty sweep)
                                      └─ buck_pwm_set_duty → HW PWM slice 0
```

Core 0는 아직 아무 실시간 일을 하지 않음. PWM HW는 Core 1이 `buck_pwm_set_duty()`를 호출해서 slice 레지스터를 쓰면 HW가 자율 동작.

### Phase 5 완성 후 목표
```
Core 0 (bare-metal RT)          Core 1 (FreeRTOS)
├─ ADC DMA + 평균               ├─ lcd_task    (10 Hz)
├─ 8.5 kHz 컨트롤 ISR          ├─ bms_task    (1 Hz, BQ34Z100)
├─ PI + CC/CV 모드 전환         ├─ settings_task (EEPROM)
├─ buck_pwm_set_duty() 호출     ├─ input_task   (버튼)
├─ 안전 (OCP/OVP)              ├─ learn_task   (학습 머신)
├─ 하드 OCP NMI                 ├─ uart_task    (UART1 외부 통신)
└─ FreeRTOS API 호출 금지       └─ log_task     (USB CDC)
```

**이동 작업 (Phase 5에서)**:
- `pwm_task`의 sweep 로직 제거, 대신 Core 0 컨트롤 ISR이 `buck_pwm_set_duty()`를 직접 호출
- `hal_pwm` / `buck_pwm` 계층은 변경 없음 (core-agnostic)
- `pwm_task`는 제거하거나 단순 init만 수행하는 setup task로 축소

### Pico SDK + FreeRTOS 통합
- FreeRTOS-Kernel: https://github.com/raspberrypi/FreeRTOS-Kernel
- Pico 1 / Pico 2 모두 동일 task 코드 (port 자동 선택)
- `configNUMBER_OF_CORES = 1` (Core 1만 RTOS, Core 0는 bare-metal)
- Core 0가 `multicore_launch_core1(core1_freertos_entry)` 호출
- Core 1이 `vTaskStartScheduler()` 호출 → 영구 RTOS 동작

데이터 교환:
- **측정 스냅샷**: `measurement_snapshot_t snap_buf[2]` + `volatile uint32_t snap_idx`. Core 0가 inactive에 쓰고 atomic swap. Core 1 task가 필요 시 active 인덱스 read.
- **명령 큐**: `multicore_fifo` (8 entry HW FIFO) 또는 spinlock-protected ring buffer. Core 1 → Core 0.

금지:
- Core 0에서 `printf`, `malloc`, LCD/I²C 접근 절대 금지
- **Core 0에서 FreeRTOS API 호출 금지** (RTOS는 Core 1만 인지)
- 공유 변수는 atomic 또는 double-buffer
- DMA 채널 분리: ADC DMA = Core 0, LCD/EEPROM/UART DMA = Core 1

---

## 11. 다음 작업 순서 (제안)

| 우선순위 | Phase | 작업 | 상태 |
|---|---|---|---|
| ✅ | **Phase 1** | LCD 드라이버 + 7 페이지 UI + 10 Hz 렌더 | **완료** |
| ✅ | **Phase 2** | SDK 2.1.1 + Pico 1/2 듀얼 타겟 빌드 | **완료** |
| ✅ | **Phase 5 일부** | FreeRTOS Core 1 포팅 + `lcd_task` | **완료** |
| ✅ | **Phase 4 일부** | PWM HAL + `buck_pwm` + `pwm_task` (85 kHz) | **완료** |
| 🔒 | (**Pico 프로젝트 종결**) | MCU를 MKV31로 변경, 이 repo는 레퍼런스로 동결 | — |
| ⭐ | **Phase 11** | **새 세션 — MKV31F512 이식**: [mkv31_migration.md](mkv31_migration.md) Checkpoint 1~10 | **다음 세션에서 시작** |
| 참고 | **Phase 3** | HAL 나머지 (`hal_adc`, `hal_i2c`, `hal_uart`) — MKV31 포트에서 작성 | Phase 11 내부 작업 |
| 3 | **Phase 4 나머지** | AD7606 SPI+DMA 드라이버 + ring buffer + 평균 | 4~6 시간 |
| 4 | **Phase 7a** | 24LC256 EEPROM 드라이버 + settings 모듈 | 3~4 시간 |
| 5 | **Phase 5 나머지** | PI 컨트롤러 + CC/CV 머신 + Core 0 8.5 kHz ISR | 6~10 시간 |
| 6 | **Phase 6** | SW OCP/OVP/OTP + watchdog (HW OCP는 보드측) | 2~3 시간 |
| 7 | **Phase 7b** | BQ34Z100 I²C 드라이버 | 4~6 시간 |
| 8 | **Phase 8** | 학습 사이클 상태 머신 | 4~6 시간 |
| 9 | **Phase 9** | 시뮬 데이터 → 실측 스냅샷 교체 | 2~3 시간 |
| 9b | **Phase 9b** | UART1 통신 (uart_task + 프로토콜) | 4~6 시간 (병렬 가능) |
| 10 | **Phase 10** | 버튼 입력 HAL + UI 디스패처 | 2~3 시간 |
| 11 | **Phase 11** | (먼 미래) MKV31 이식 | 1~2 주 |

---

## 12. 작업을 다시 시작할 때

다른 세션에서 이 프로젝트를 이어받으면:

1. **이 문서 + [design_guide.md](design_guide.md) 먼저 읽기**
2. `git log --oneline` 으로 최근 진행상황 확인
3. `build.cmd` 실행해서 빌드 동작 확인
4. Pico에 굽고 LCD UI 데모가 잘 도는지 확인
5. 위 § 11 우선순위 표 보고 다음 작업 결정

### 자주 쓰는 명령
```bash
# 빌드 (SDK 2.x, 권장)
cmd //c "e:\ONEDRIVE\WORK\01_Developmemt\07_CANTOPS\260410_GLCD\build-v2.cmd pico"
cmd //c "e:\ONEDRIVE\WORK\01_Developmemt\07_CANTOPS\260410_GLCD\build-v2.cmd pico2"

# 클린 빌드
cmd //c "e:\ONEDRIVE\WORK\01_Developmemt\07_CANTOPS\260410_GLCD\build-v2.cmd pico clean"
cmd //c "e:\ONEDRIVE\WORK\01_Developmemt\07_CANTOPS\260410_GLCD\build-v2.cmd pico2 clean"

# 빌드 (SDK 1.5.1 레거시)
cmd //c "e:\ONEDRIVE\WORK\01_Developmemt\07_CANTOPS\260410_GLCD\build.cmd"

# git 상태
git status
git log --oneline -10
```

### 코드 수정 시 주의
- LCD/UI 코드는 Phase 1 완료 상태이므로 함부로 건드리지 말 것 (regression 위험)
- 새 모듈은 `src/<module>/`에 추가하고 `CMakeLists.txt`의 SRC_* 변수에 등록
- HAL을 거치는 형태 유지 (port-specific 코드는 절대 lcd/ui/control 안에 들어가면 안 됨)
- float 연산 OK (Pico 1 ROM, Pico 2 FPU). fixed-point는 필요 없을 때까지 미루기.

### 현재 미해결 사항 (의도적으로 남겨둔 것)
- `src/GLCD/` 빈 폴더가 있음 (사용자가 만든 듯). 그대로 두기. 필요하면 사용자에게 확인 후 삭제.
- **실 Pico 2 보드 교체는 아직 미수행** — UF2는 준비됨, 보드 도착 시 바로 굽기 가능
- **UART1 용도 / 프로토콜 / baud rate 미정** — Phase 9b 들어가기 전 사용자에게 확인

---

## 13. 알려진 함정

| 함정 | 해결 |
|---|---|
| `printf` from Core 0 | Core 1로 보내거나 USB stdio off |
| 4-gray 모드에서 mono 페이지 수 사용 | LCD_PAGES = 40 (4-gray), 20 (mono) |
| LCD 비트 순서 LSB-top 가정 | MSB-top: `1 << (7 - y%8)` |
| Vop high byte 변경 | init과 동일하게 유지 |
| pico-env.cmd PICO_INSTALL_PATH 없음 | local wrapper가 미리 설정 |
| Float on Pico 1 fast loop | 1 kHz~10 kHz는 OK, 그 이상은 fixed-point 또는 Pico 2 |
| Core 0에서 플래시 erase/write | XIP 중단됨, EEPROM(외부) 사용으로 회피 |
| Core 0에서 FreeRTOS API 호출 | **금지** — RTOS는 Core 1만 인지 |
| SPI shared between LCD and ADC | LCD = SPI0, ADC = SPI1 분리 |
| 1S 셀 방전 한계 | 3S~6S만 지원, 1S/2S는 처음부터 제외 |
| UART 핀 충돌 | GP8/GP9 사용, 버튼 TAB/UP은 GP27/GP28로 이동 |

---

## 14. GitHub
- Repo: https://github.com/msick2/GLCD
- 첫 커밋: 2026-04-11
- 브랜치: `main`
- 32 파일, build/ 제외

푸시 / 풀:
```bash
git pull origin main
git push origin main
```

---

## 15. 연락처 / 외부 의존성

- **bqStudio** (TI 무료): BQ34Z100 chemistry 사전 등록용. 이 소프트웨어는 EV2300/EV2400 USB-I²C 어댑터가 필요.
- **Pico SDK 1.5.1** (레거시): `C:\Program Files\Raspberry Pi\Pico SDK v1.5.1` — pico-setup-windows 1.5.1 installer 사용, 툴체인/CMake/Ninja/Python 제공
- **Pico SDK 2.1.1** (현재, 권장): `E:\pico-sdk-2\sdk` — git shallow clone + 서브모듈 (raspberrypi/pico-sdk @ 2.1.1)
- **picotool 2.2.0-a4**: `E:\pico-sdk-2\tools\picotool` — RP2350 UF2 family 지원
  - 다운로드: https://github.com/raspberrypi/pico-sdk-tools/releases/tag/v2.2.0-3

### SDK 2.x 재설치 (필요 시)
```bash
mkdir -p /e/pico-sdk-2 && cd /e/pico-sdk-2
git clone --branch 2.1.1 --depth 1 https://github.com/raspberrypi/pico-sdk.git sdk
cd sdk && git submodule update --init --depth 1

mkdir -p ../tools && cd ../tools
curl -sL -o picotool.zip https://github.com/raspberrypi/pico-sdk-tools/releases/download/v2.2.0-3/picotool-2.2.0-a4-x64-win.zip
unzip picotool.zip
```

---

이 문서를 갱신할 때는 마지막 업데이트 날짜와 변경 항목을 기록해주세요.
