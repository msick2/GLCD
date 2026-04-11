# MKV31F512 이식 핸드오프 문서

작성: 2026-04-11
출처 프로젝트: https://github.com/msick2/GLCD (Pico 기반 레퍼런스)
타겟 프로젝트: **새 레포지토리 / 새 세션**

---

## 0. 이 문서의 목적

이 repo (`msick2/GLCD`)에서 Pico (RP2040/RP2350) 타겟으로 개발된 LCD/UI/FreeRTOS/PWM 코드가 동작 검증된 상태입니다. 그러나 실제 PCB 제품은 **NXP MKV31F512VLH12**로 만들기로 결정했습니다.

이 문서는 **새 세션을 열 사람 (또는 새로운 Claude 인스턴스)** 이 Pico repo를 레퍼런스로 삼아 MKV31 프로젝트를 처음부터 세팅할 수 있도록 모든 맥락을 전달합니다.

**전제**: 당신이 이 문서를 처음 읽는다면, 먼저 [design_guide.md](design_guide.md) 를 전체 훑어보고 프로젝트의 큰 그림을 잡으세요. 이 문서는 "그러면 새 세션에서 구체적으로 뭘 해야 하는가"만 다룹니다.

---

## 1. 프로젝트 한 줄 요약

BQ34Z100 퓨얼게이지 IC의 학습 사이클을 자동화하는 충방전 보드. 48V 입력, 3S~6S Li-ion, 충전 벅 + 방전 벅 (비동기 buck, 85 kHz PWM), 256×160 4-gray LCD UI, 외부 BQ34Z100 보드와 I²C 통신.

---

## 2. 하드웨어 확정 사항

### MCU
| 항목 | 값 |
|---|---|
| 부품 | **NXP MKV31F512VLH12** |
| 패키지 | LQFP-64 |
| 코어 | Cortex-M4 + FPU |
| 클럭 | 120 MHz (내부 PLL, 4 MHz 크리스탈 × 30) |
| 플래시 | 512 KB |
| SRAM | 96 KB |
| USB | 없음 (디버그는 UART ↔ J-Link VCOM) |

### 주변기기 사용
- **FlexTimer**: 2× 85 kHz PWM (충전 벅 + 방전 벅), ADC 하드웨어 트리거
- **ADC0 + ADC1**: 16-bit, 6채널 측정 (VIN, IIN, VBAT, I_CHG, I_DCHG, VLOAD), 외부 Vref 3.0 V (REF3030)
- **DSPI**: LCD (ST75256)
- **I²C0**: BQ34Z100 (외부 보드)
- **I²C1**: 24LC256 EEPROM (로컬)
- **LPUART 또는 UART**: 디버그 로그 (J-Link VCOM)
- **GPIO**: 5 버튼, 2 gate driver enable, 2 OCP (fault in / latch reset), 1~2 LED

### 전원/전력 섹션 (PCB 전체 BOM은 design_guide.md § 15 참조)
- 48V 입력 → 충전 벅 (비동기, NCP51313 + NMOS + Schottky, 100 µH) → 3S~6S 배터리
- 배터리 → 방전 벅 → 2.5 Ω 40 W dump load (10 Ω 10 W × 4 병렬)
- 전류 센서: **ACS725LLCTR-10AU-S × 3** (Hall, 5V 공급, 200 mV/A, 0.5V offset)
- 전압 분압: 0.1% 박막 저항
- HW OCP: LM393 + 74HC279 SR latch + 74HC08 AND 게이트
- BQ34Z100은 **외부 보드**, I²C 케이블로 연결

### 디버거
- **J-Link** (사용자 보유)
- SWD 커넥터: 10-pin 1.27 mm 또는 20-pin 2.54 mm
- UART를 J-Link VCOM에 연결하면 디버그 로그가 PC COM 포트로 출력됨

---

## 3. 소프트웨어 재사용 정책

### 그대로 복사 (MCU 무관 코드)

이 repo에서 **수정 없이 그대로 복사**할 디렉터리:

```
src/
├── lcd/
│   ├── lcd.h
│   ├── lcd_st75256.c         ← MSB-top, 4-row/page, init 시퀀스 검증 완료
│   └── lcd_st75256_cmds.h
├── ui/
│   ├── font5x7.[ch]          ← 5x7 ASCII 47 글자
│   ├── draw.[ch]             ← hline/vline/box/text/text_right
│   ├── sim.[ch]              ← SS[6] 시뮬 상태 (나중에 실측으로 교체)
│   └── ui.[ch]               ← 탭 + 헤더 + 7 페이지 렌더러
├── pwm/
│   └── buck_pwm.[ch]         ← BUCK_CHARGE / BUCK_DISCHARGE wrapper
├── freertos_config/
│   └── FreeRTOSConfig.h      ← M33 매크로는 무시되고 M4 매크로만 유효
└── tasks/
    ├── core1_entry.[ch]      ← 이름은 "core1"이지만 MKV31에서는 단일 코어 entry로 개명 권장
    ├── lcd_task.[ch]         ← 10 Hz 렌더 (vTaskDelayUntil)
    └── pwm_task.[ch]         ← buck PWM init + sweep demo (나중에 제어 루프로 교체)
```

HAL 헤더도 그대로 사용:
```
src/hal/
├── hal_spi.h
├── hal_gpio.h
├── hal_delay.h
└── hal_pwm.h
```

### 새로 작성 (MKV31 포트)

```
src/port/mkv31/
├── board_config.h            ← 핀, DSPI 포트, FlexTimer 채널, 주파수
├── hal_spi_mkv31.c           ← DSPI (LCD용)
├── hal_gpio_mkv31.c          ← GPIO + PORT mux
├── hal_delay_mkv31.c         ← SysTick 기반
├── hal_pwm_mkv31.c           ← FlexTimer (FTM) 85 kHz
├── hal_adc_mkv31.c           ← ADC0/ADC1 + DMA + FlexTimer 트리거
├── hal_i2c_mkv31.c           ← I2C0 (BQ) + I2C1 (EEPROM)
└── hal_uart_mkv31.c          ← LPUART debug log
```

### 버릴 것

```
src/port/pico/      ← 전부 버림 (새 프로젝트에선 필요 없음)
src/app/main.c      ← 새로 작성 (MKV31은 단일 코어)
CMakeLists.txt      ← 새로 작성 (Pico SDK → MKV31 SDK로 교체)
pico_sdk_import.cmake, pico-env*.cmd, build*.cmd  ← 전부 버림
```

### HAL 변경 사항 (다음 세션에서 추가해야 할 것)

현재 HAL은 `hal_spi`, `hal_gpio`, `hal_delay`, `hal_pwm`만 있습니다. MKV31 이식 시 추가로 작성:
- `hal_adc.h` (신규) — MKV31 내장 ADC 추상화
- `hal_i2c.h` (신규)
- `hal_uart.h` (신규)

이 세 헤더는 먼저 "사용처가 원하는 인터페이스"를 기준으로 설계한 후, `port/mkv31/hal_*_mkv31.c`에서 MCUXpresso SDK API로 구현합니다.

---

## 4. 개발 환경 셋업 (Checkpoint 1)

### 4.1 MCUXpresso SDK 다운로드
1. https://mcuxpresso.nxp.com/ 접속, NXP 계정으로 로그인
2. SDK Builder 클릭
3. "Select Development Board" → "Processors" 탭 → **MKV31F51212** 선택
4. Toolchain: **GCC ARM Embedded**
5. Host OS: **Windows**
6. 선택 옵션:
   - ✅ FreeRTOS
   - ✅ CMSIS DSP
   - ✅ 기본 드라이버 (fsl_gpio, fsl_dspi, fsl_ftm, fsl_adc16, fsl_i2c, fsl_lpuart 등)
7. Download SDK 클릭 → ~300 MB zip

zip을 `E:\mkv31-sdk\` 같은 위치에 풀어둡니다 (경로에 공백 없게).

### 4.2 MCUXpresso IDE 설치
1. https://www.nxp.com/mcuxpresso-ide 에서 Windows 버전 다운로드
2. 설치 (기본 옵션)
3. 처음 실행 시 SDK 가져오기: `Installed SDKs` 뷰에 `E:\mkv31-sdk\SDK_x.x_MKV31F51212` 폴더를 드래그

### 4.3 빈 프로젝트 생성 (Config Tool)
1. File → New → Create a new C/C++ project
2. "MKV31F51212" 보드 선택 (또는 해당 프로세서)
3. "Import SDK example(s)"에서 `hello_world` 템플릿 가져오기
4. FreeRTOS 포함 옵션 체크
5. 생성 후 `board/` 폴더에 Pin Mux + Clock 설정 파일 생성됨

**중요**: 이 단계의 목적은 MCUXpresso Config Tool이 생성해주는 **클럭/핀 초기화 코드**를 얻는 것입니다. 빌드가 돌면 그 설정 파일들 (`board.c/h`, `clock_config.c/h`, `pin_mux.c/h`)을 나중에 CMake 프로젝트로 복사합니다.

### 4.4 클럭 설정 (Config Tool)
- **외부 크리스탈**: 4 MHz
- **MCG 모드**: FEI → FEE → PEE
- **PLL**: 4 MHz × 30 = 120 MHz (VCO), /2 = 60 MHz 또는 바로 120 MHz 시스템 클럭
- **Core clock**: 120 MHz
- **Bus clock**: 60 MHz (일반적)
- **FlexBus clock**: (사용 안 함)
- **Flash clock**: 24 MHz (max)

Config Tool GUI에서 드래그하면 `clock_config.c`가 자동 생성됩니다.

### 4.5 핀 설정 (Pin Mux Tool)
§ 2의 주변기기 목록을 기준으로 핀을 할당합니다. ADC 입력은 ADC-capable 핀(PTB, PTC 일부)에만 배치 가능. FlexTimer는 FTM0_CH0~CH7이 각각 특정 PTA/PTC 핀에 고정.

**핀 결정 순서**:
1. 전원/크리스탈/SWD/RESET/VREFH 고정
2. ADC 6채널 (ADC-capable 핀만)
3. FTM0 채널 2개 (충전/방전 PWM)
4. DSPI0 (LCD) 5핀
5. I2C0 (BQ) 2핀, I2C1 (EEPROM) 2핀
6. LPUART (디버그) 2핀
7. 나머지 GPIO (버튼 5, enable 2, OCP 2, LED 1~2)

결과 `pin_mux.c`를 확보합니다.

### 4.6 J-Link 디버깅 동작 확인
`hello_world` 빌드 → J-Link 연결 → Debug 실행. 브레이크포인트가 걸리고 단일 스텝 실행이 되면 ✅.

### Checkpoint 1 완료 조건
- MCUXpresso IDE에서 MKV31 hello_world가 120 MHz로 동작
- J-Link 디버깅 가능
- Pin Mux Tool에서 § 2 주변기기 모두 배치됨
- `board.c/h`, `clock_config.c/h`, `pin_mux.c/h`, startup, linker script 파일 확보

---

## 5. CMake + VS Code 전환 (Checkpoint 2)

MCUXpresso IDE의 Eclipse 빌드는 일단 버리고, 우리 기존 CMake 구조로 전환합니다.

### 5.1 새 repo 생성
```
C:/dev/glcd-mkv31/     (또는 원하는 경로)
├── CMakeLists.txt     ← 새로 작성
├── docs/
│   ├── design_guide.md  ← 이 repo에서 복사
│   └── mkv31_migration.md  ← 이 파일 복사
├── src/
│   ├── app/main.c     ← 새로 작성
│   ├── lcd/           ← 이 repo에서 복사
│   ├── ui/            ← 이 repo에서 복사
│   ├── pwm/           ← 이 repo에서 복사
│   ├── tasks/         ← 이 repo에서 복사 (core1_entry → board_entry 개명 권장)
│   ├── freertos_config/FreeRTOSConfig.h  ← 이 repo에서 복사
│   ├── hal/           ← hal_*.h 복사 (+ hal_adc.h, hal_i2c.h, hal_uart.h 신규)
│   └── port/mkv31/
│       ├── board_config.h   ← 새로 작성
│       ├── startup_MKV31F51212.S   ← MCUXpresso에서 복사
│       ├── MKV31F51212xxx12_flash.ld  ← MCUXpresso에서 복사
│       ├── clock_config.[ch]  ← MCUXpresso Config Tool 결과
│       ├── pin_mux.[ch]       ← MCUXpresso Config Tool 결과
│       ├── board.[ch]         ← MCUXpresso Config Tool 결과
│       └── hal_*_mkv31.c      ← 새로 작성
└── sdk/               ← MCUXpresso SDK 드라이버 파일만 복사 또는 symlink
    ├── CMSIS/
    ├── drivers/
    └── device/
```

### 5.2 CMakeLists.txt 골격

```cmake
cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/arm-none-eabi.cmake)
project(glcd C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MCU MKV31F51212)
set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

# SDK source list
set(SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/sdk)
file(GLOB SDK_DRIVERS
    ${SDK_DIR}/drivers/fsl_gpio.c
    ${SDK_DIR}/drivers/fsl_port.c
    ${SDK_DIR}/drivers/fsl_dspi.c
    ${SDK_DIR}/drivers/fsl_ftm.c
    ${SDK_DIR}/drivers/fsl_adc16.c
    ${SDK_DIR}/drivers/fsl_i2c.c
    ${SDK_DIR}/drivers/fsl_lpuart.c
    ${SDK_DIR}/drivers/fsl_clock.c
    ${SDK_DIR}/drivers/fsl_common.c
    ${SDK_DIR}/drivers/fsl_smc.c
    # ...
)

# FreeRTOS (single-core ARM_CM4F port)
file(GLOB FREERTOS_SRC
    ${SDK_DIR}/freertos/freertos-kernel/*.c
    ${SDK_DIR}/freertos/freertos-kernel/portable/GCC/ARM_CM4F/port.c
    ${SDK_DIR}/freertos/freertos-kernel/portable/MemMang/heap_4.c
)

# Application source
set(APP_SRC
    src/app/main.c
    src/lcd/lcd_st75256.c
    src/ui/font5x7.c src/ui/draw.c src/ui/sim.c src/ui/ui.c
    src/pwm/buck_pwm.c
    src/tasks/board_entry.c src/tasks/lcd_task.c src/tasks/pwm_task.c
    src/port/mkv31/hal_spi_mkv31.c
    src/port/mkv31/hal_gpio_mkv31.c
    src/port/mkv31/hal_delay_mkv31.c
    src/port/mkv31/hal_pwm_mkv31.c
    src/port/mkv31/hal_adc_mkv31.c
    src/port/mkv31/hal_i2c_mkv31.c
    src/port/mkv31/hal_uart_mkv31.c
    src/port/mkv31/clock_config.c
    src/port/mkv31/pin_mux.c
    src/port/mkv31/board.c
    src/port/mkv31/startup_MKV31F51212.S
)

add_executable(glcd
    ${APP_SRC} ${SDK_DRIVERS} ${FREERTOS_SRC}
)

target_include_directories(glcd PRIVATE
    src/app src/lcd src/ui src/pwm src/tasks src/freertos_config
    src/hal src/port/mkv31
    ${SDK_DIR}/CMSIS/Core/Include
    ${SDK_DIR}/devices/MKV31F51212
    ${SDK_DIR}/drivers
    ${SDK_DIR}/freertos/freertos-kernel/include
    ${SDK_DIR}/freertos/freertos-kernel/portable/GCC/ARM_CM4F
)

target_compile_options(glcd PRIVATE ${CPU_FLAGS} -O2 -g3 -Wall -ffunction-sections -fdata-sections)
target_compile_definitions(glcd PRIVATE CPU_${MCU} __USE_CMSIS)

target_link_options(glcd PRIVATE
    ${CPU_FLAGS}
    -T${CMAKE_CURRENT_LIST_DIR}/src/port/mkv31/MKV31F51212xxx12_flash.ld
    -Wl,--gc-sections -Wl,-Map=glcd.map
    --specs=nano.specs --specs=nosys.specs
)

# Generate binary and hex
add_custom_command(TARGET glcd POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary glcd glcd.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex glcd glcd.hex
    COMMAND ${CMAKE_SIZE} glcd
)
```

### 5.3 arm-none-eabi.cmake toolchain 파일
```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE INTERNAL "")
set(CMAKE_SIZE arm-none-eabi-size CACHE INTERNAL "")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
```

### 5.4 FreeRTOSConfig 조정
이 repo의 [src/freertos_config/FreeRTOSConfig.h](../src/freertos_config/FreeRTOSConfig.h)는 M33 (RP2350) 매크로가 있습니다. M4F에서도 문제없이 동작하지만, 다음을 명시적으로:
- `configCPU_CLOCK_HZ`를 **120000000** 으로 변경
- `configKERNEL_INTERRUPT_PRIORITY`와 `configMAX_SYSCALL_INTERRUPT_PRIORITY`를 M4F 기준으로 재설정 (4비트 우선순위 사용, `(15 << 4)` / `(5 << 4)` 같은 값)
- `configENABLE_FPU = 1` 은 RP2350 전용 매크로이므로 M4F에서 무시되지만, 컴파일러 플래그 `-mfpu=fpv4-sp-d16 -mfloat-abi=hard`로 FPU 하드웨어 활성화됨

### 5.5 main.c 단순 템플릿
```c
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "FreeRTOS.h"
#include "task.h"

extern void board_entry(void);   /* 구 core1_entry, 개명 */

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    board_entry();               /* LCD init + tasks + vTaskStartScheduler */
    for (;;) { }                 /* unreachable */
}
```

### Checkpoint 2 완료 조건
- VS Code + CMake + Ninja로 빈 main.c가 빌드됨
- `glcd.elf`/`glcd.bin` 생성
- J-Link로 굽기 성공 (MCUXpresso IDE가 아닌 CLI/VS Code에서)
- 부팅 시 fault 없음 (BusFault/HardFault로 떨어지지 않음)
- Blinky LED가 깜빡이면 최고 (GPIO 동작 확인)

---

## 6. HAL 이식 (Checkpoint 3~8)

각 HAL을 한 번에 하나씩 이식합니다. 각 단계마다 작은 테스트를 해서 동작 확인 후 넘어갑니다.

### Checkpoint 3: `hal_gpio_mkv31.c` + `hal_delay_mkv31.c`
- MCUXpresso SDK `fsl_gpio.h`, `fsl_port.h` 사용
- `hal_gpio_init(pin, dir)`: PORT_SetPinMux + GPIO_PinInit
- `hal_gpio_write/read`: GPIO_PinWrite / GPIO_PinRead
- `hal_delay_us/ms`: SysTick 기반 또는 `SDK_DelayAtLeastUs`
- **테스트**: LED를 GPIO로 직접 깜빡이기

### Checkpoint 4: `hal_spi_mkv31.c` → LCD 동작
- MCUXpresso SDK `fsl_dspi.h` 사용
- `hal_spi_init(cfg)`: DSPI_MasterInit
- `hal_spi_write(buf, len)`: DSPI_MasterTransferBlocking
- **테스트**: `lcd_init()` 호출 → `lcd_test_fill_4gray()` 로 화면이 4단계 회색으로 바뀌는지 확인
- **주의 사항** (design_guide.md § 6 참조):
  - ST75256 MSB-top 비트 순서
  - 4-gray 모드 page = 4 rows
  - 0x5C write data 명령 필수
  - Vop = 0x24/0x04

### Checkpoint 5: UI 데모 동작
- `core1_entry` → `board_entry`로 파일명 개명 후 포함
- `lcd_task` 생성 + `vTaskStartScheduler`
- **테스트**: 전체 UI 데모가 10 Hz로 동작, 프레임 카운터가 증가

### Checkpoint 6: `hal_pwm_mkv31.c` (FlexTimer 85 kHz)
- MCUXpresso SDK `fsl_ftm.h` 사용
- `hal_pwm_init(ch, cfg)`: FTM 채널 설정, 120 MHz / 85 kHz = 1412 top value
- `hal_pwm_set_duty(ch, duty)`: FTM_UpdatePwmDutycycle
- **테스트**: `pwm_task`로 두 채널 sweep, 스코프로 GP?에서 85 kHz 확인

### Checkpoint 7: `hal_adc_mkv31.c` (ADC0/ADC1 + DMA + FlexTimer trigger)
- 가장 어려운 부분. `fsl_adc16.h`, `fsl_dma.h` 조합
- 먼저 단순 polling으로 ADC16_DoAutoCalibration + ADC16_SetChannelConfig로 1채널 읽기
- 그 다음 DMA 연결 + FlexTimer trigger 추가
- **테스트**: 알려진 전압 (예: 3.3V / 2, 1.65V)을 하나 입력해서 ADC가 중간값을 읽는지 확인

### Checkpoint 8: `hal_i2c_mkv31.c` + `hal_uart_mkv31.c`
- `fsl_i2c.h`, `fsl_lpuart.h`
- I²C: BQ34Z100 더미 read (device ID 읽기)
- UART: `printf` redirect → J-Link VCOM으로 로그 출력
- **테스트**: PC 터미널에서 로그 보이면 ✅

---

## 7. 모듈 구현 (Checkpoint 9~10)

### Checkpoint 9: buck_pwm + 간단한 PI (open loop)
- `buck_pwm_init()` 호출 확인 (FlexTimer 시작)
- ADC로 측정한 값을 그냥 UART에 출력 (아직 피드백 루프 없음)
- duty를 수동으로 바꿔가며 측정값이 반응하는지 확인

### Checkpoint 10: 8.5 kHz 컨트롤 루프 + CC/CV 상태 머신
- FlexTimer INIT_TRIG → ADC trigger → DMA → 10 샘플 ring buffer → 8.5 kHz ADC 완료 ISR
- ISR에서 PI step + PWM duty 갱신
- CC/CV 모드 머신 추가
- 안전 체크 (OCP/OVP/UVP)
- 단순 학습 사이클: IDLE → CC charge → CV → REST → CC discharge → REST → 반복

이 시점에서 학습 보드 핵심 기능 완성.

---

## 8. 기타 모듈 (Checkpoint 이후)

- **EEPROM 드라이버** (24LC256) + settings 모듈
- **BQ34Z100 드라이버** (I²C 명령 wrapping, chemistry는 bqStudio로 사전 등록됨)
- **학습 사이클 상태 머신** (learn_task)
- **UI 실데이터 연결** (시뮬 SS 배열 → measurement_snapshot_t)
- **버튼 입력** (GPIO 폴링 + 디바운스 + FreeRTOS queue)
- **UART1 외부 통신 프로토콜** (용도 미정 — 사용자 협의 필요)

---

## 9. 핵심 주의 사항 & 함정

이 repo에서 발견한 함정 목록. 새 세션에서 시간 절약용.

### ST75256 LCD (design_guide.md § 6, memory.md § 8)
1. **MSB-top 비트 순서**: 페이지 내 bit 7 = 윗 행. `1 << (7 - y%8)`.
2. **4-gray 모드는 page = 4 rows** (mono는 8). LCD_PAGES 상수를 혼동하지 말 것.
3. **0x5C Write Data 명령 필수**: 페이지/컬럼 설정 후 0x5C 안 보내면 RAM에 안 들어감.
4. **CS는 cmd+data transaction 동안 low 유지** 권장.
5. **`lcd_set_contrast()`의 high byte는 init과 같아야 함**. 다르면 무시됨. 0x04로 통일.
6. **Vop 인코딩 반전** 가능성: 실측으로 0x24 / 0x04가 최적.

### FreeRTOS on M4F
- `configCPU_CLOCK_HZ = 120000000` 설정
- `configKERNEL_INTERRUPT_PRIORITY = (15 << (8 - 4))` (M4F는 우선순위 4비트)
- `configMAX_SYSCALL_INTERRUPT_PRIORITY = (5 << (8 - 4))` — 이것보다 우선순위 낮은(숫자 큰) IRQ에서만 FreeRTOS `FromISR` API 호출 가능
- 8.5 kHz 컨트롤 ISR은 `configMAX_SYSCALL_INTERRUPT_PRIORITY` **보다 높은** 우선순위(숫자 작은)에 배치 → FreeRTOS API 호출 금지, 공유 변수만 사용

### MKV31 FlexTimer
- FTM_CHnV 레지스터에 duty 값 직접 쓰기 가능
- FTM_SYNC를 통해 duty 갱신이 다음 wrap에서 atomic 적용되도록 설정
- FTM → ADC trigger는 `TRIG_INIT` 또는 `CH_TRIG` 사용 (FlexTimer와 ADC16 데이터시트 교차 확인)
- Cortex-M4 NVIC의 우선순위는 4비트 (값 0~15), 0이 최고

### MKV31 ADC
- **부팅 시 `ADC16_DoAutoCalibration()` 반드시 호출** — 생략하면 gain/offset 오차 큼
- 외부 VREFH/VREFL 사용 시 `ADC16_SetHardwareAverage` 와 `ADC16_EnableDMA` 설정 순서 주의
- Differential 모드와 single-ended 구분

### 전원 투입 순서
- 48V → 12V → 5V → 3.3V → MKV31 VDD
- MKV31 VDDA와 VREFH는 디커플링 캡 여러 개 (10 µF + 100 nF + 10 nF)
- REF3030 출력을 VREFH에 직접 연결 (decoupling cap 포함)

---

## 10. 미결정 사항 (새 세션에서 결정 필요)

| # | 항목 | 참고 |
|---|---|---|
| 1 | **MKV31 핀 할당 확정** | Pin Mux Tool에서 결정 → board_config.h에 기록 |
| 2 | **LED 개수 / 위치** | 상태 LED 1개 또는 여러 개 (I²C GPIO expander 옵션도 있음) |
| 3 | **UART1 용도 / 프로토콜 / baud** | PC통신? RS-485? 외부 마스터? 이전 세션에서도 미결정 |
| 4 | **버튼 물리 구성** | 멤브레인? 택트 스위치? 커넥터? |
| 5 | **SWD 커넥터 풋프린트** | 10핀 1.27 mm (ARM standard) 추천 |
| 6 | **Vref 정밀도** | REF3030 ±0.2% vs REF3440 ±0.05% (더 정밀) |
| 7 | **PCB 레이어 수** | 4층 권장 (파워 + 디지털 + 아날로그 그라운드 분리) |
| 8 | **케이스 / 방열** | 벅 FET + dump load 방열 전략 |

---

## 11. 체크리스트 요약

새 세션 작업 순서:

- [ ] **Checkpoint 1**: MCUXpresso SDK 설치 + IDE에서 hello_world 동작 + Config Tool 결과 확보
- [ ] **Checkpoint 2**: VS Code + CMake + J-Link로 빈 main.c 빌드/굽기 성공, LED 점멸
- [ ] **Checkpoint 3**: `hal_gpio`, `hal_delay` 이식 + LED
- [ ] **Checkpoint 4**: `hal_spi` 이식 + LCD `lcd_test_fill_4gray()` 동작
- [ ] **Checkpoint 5**: 전체 UI 데모 10 Hz 동작 (Pico와 동일)
- [ ] **Checkpoint 6**: `hal_pwm` (FlexTimer) 이식 + 85 kHz sweep
- [ ] **Checkpoint 7**: `hal_adc` 이식 + DMA + FlexTimer 트리거
- [ ] **Checkpoint 8**: `hal_i2c`, `hal_uart` 이식 + J-Link VCOM 로그
- [ ] **Checkpoint 9**: buck_pwm + 수동 duty 제어 + ADC 측정
- [ ] **Checkpoint 10**: 8.5 kHz 컨트롤 루프 + CC/CV + 간단 학습 사이클
- [ ] (이후) EEPROM, BQ34Z100, 버튼 입력, UART 프로토콜, UI 실데이터 연결

각 Checkpoint 완료 시 **commit + 간단한 테스트 설명**을 남기면 이어받기가 수월합니다.

---

## 12. 참고 링크

- 이 프로젝트 (Pico 레퍼런스): https://github.com/msick2/GLCD
- MCUXpresso SDK: https://mcuxpresso.nxp.com/
- MKV31F512 데이터시트: https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/kv-series-cortex-m4-m0-plus-m7/kinetis-kv31-120-mhz-performance-motor-control-microcontrollers-mcus-based-on-arm-cortex-m4-core:KV31_120
- FreeRTOS Cortex-M4F 포트: https://www.freertos.org/RTOS-Cortex-M3-M4.html
- J-Link 소프트웨어: https://www.segger.com/downloads/jlink/

---

## 13. 이 문서 변경 이력

| 날짜 | 변경 |
|---|---|
| 2026-04-11 | 초안. Pico repo의 상태를 MKV31 새 세션에 인계하기 위한 완전한 핸드오프 문서. Checkpoint 1~10 제시. |
