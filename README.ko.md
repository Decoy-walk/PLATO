# PLATO — 물질적 상호작용을 통한 체화된 기억(Embodied Memory)

*[English](README.md)*

손과 물리적 폴딩 블록 장치 간의 촉각-운동(tactile-motor) 상호작용이 절차기억
(procedural memory)을 보조 채널로 활성화시켜, text-only 인코딩 대비 회상을
향상시키는지를 검증하는 연구용 프로토타입입니다. 폴딩 블록에 내장된 센싱
노드가 20개 힌지 전체를 추적하고, fold 상태를 유선 USB serial로 PC 브릿지에
스트리밍하면 브릿지가 이를 CSV, git, 그리고/또는 Google Sheet에 기록합니다.

```
 손  ⇄  폴딩 블록 (flex 센서 힌지 20개 + 햅틱 액추에이터)
                 │  USB serial (~20 Hz, 프레임된 4바이트 fold bitmask)
                 ▼
         bridge/plato_bridge.py  (PC)
                 │
        ┌────────┼─────────┐
        ▼        ▼         ▼
   data/*.csv   git commit   Google Sheets
```

## 하드웨어

| 구성 요소                          | 역할                                                    |
|-----------------------------------|------------------------------------------------------------|
| Seeed XIAO SAMD21                | MCU: 20개 힌지 스캔, 햅틱 피드백 구동, USB로 스트리밍 |
| flex 센서 20개 (힌지당 1개)       | 손으로 블록을 조작할 때 fold 시퀀스를 캡처 |
| CD74HC4067 (16채널 아날로그 멀티플렉서) 2개 | flex 센서 20채널을 ADC 핀 1개로 멀티플렉싱      |
| 햅틱/ERM 모터 + 드라이버 트랜지스터 | "Block + haptic feedback" 조건의 피드백 채널 |

보드는 원래 ESP32C3였으나(git 히스토리 참고) 영구적으로 SAMD21로 교체했습니다.
SAMD21은 온보드 라디오가 없어서 무선 로깅 방식은 아직 미해결 문제입니다 —
아래 "무선 통신은 아직 미정" 참고 — 그래서 이 빌드는 유선 USB serial 연결을
임시 전송 방식으로 사용합니다.

### ADC 핀을 늘리거나 I2C ADC 확장 보드 대신 멀티플렉서를 쓰는 이유

ESP32C3든 SAMD21의 XIAO 폼팩터든 네이티브 ADC 핀 20개를 노출하지 않습니다.
5개의 I2C ADC 보드(예: ADS1115)를 추가하는 대신, 이 설계는 채널 수를 보드가
실제로 처리할 수 있는 수준에 맞춥니다: CD74HC4067 16채널 아날로그 멀티플렉서
2개(32채널 가용, 20채널 사용)가 주소선 4개를 공유하며 flex 센서 20개를 ADC
핀 **1개**로 시분할합니다. 한 번에 한 멀티플렉서의 active-low `EN`만 LOW로
구동되고, 나머지 하나의 출력은 하이임피던스 상태를 유지하므로, 두 칩의 공통
`SIG` 핀을 같은 ADC 입력에 묶을 수 있습니다. 순비용: ADC 핀 20개나 추가 I2C
ADC 하드웨어 대신 ADC 핀 1개 + 디지털 GPIO 6개. (SAMD21은 사실 ESP32C3의
ADC2처럼 라디오와 ADC 핀이 충돌하는 제약이 없지만, 물리적 하네스와의 연속성을
위해 멀티플렉서 배선은 동일하게 유지했습니다.)

### 핀 배치 (Seeed XIAO SAMD21 실크스크린 라벨 기준)

| 기능                          | XIAO 핀  | 비고 |
|-------------------------------|----------|-------|
| 멀티플렉서 공통 아날로그 출력  | D0 (A0)  | 두 CD74HC4067의 `SIG` 핀이 여기로 묶임 |
| 멀티플렉서 주소 S0             | D1       | 두 멀티플렉서 칩이 공유 |
| 멀티플렉서 주소 S1             | D2       | |
| 멀티플렉서 주소 S2             | D3       | |
| 멀티플렉서 주소 S3             | D4       | |
| 멀티플렉서 A enable (active-low) | D5     | 힌지 0-15 선택 |
| 멀티플렉서 B enable (active-low) | D6     | 힌지 16-19 선택 (mux B의 채널 4-15는 미사용) |
| 햅틱 액추에이터 PWM            | D7       | ERM/LRA 모터를 구동하는 트랜지스터로 (+ 플라이백 다이오드) |
| 상태 LED                       | D10      | 어떤 힌지든 folded 상태로 읽히면 점등 |

각 flex 센서는 자신의 멀티플렉서 채널로 이어지는 전압분배 회로를 구성합니다:
`3V3 — flex센서 — muxChannel — 10kΩ — GND`. 두 멀티플렉서 칩의 `S0-S3`는
병렬로 배선되어 있고, `EN` 핀은 별도라서 펌웨어가 어느 16채널 뱅크가 현재
공유 `SIG`/ADC 라인을 구동할지 선택할 수 있습니다.

## 두 개의 별도 펌웨어 빌드

- `firmware/PLATO_XIAO_SAMD21/` — 아래 설명된 연구용 빌드 (SAMD21, 20-힌지
  센싱, 유선 serial, PC 브릿지).
- `firmware/Plato_001_fsr_led/` — Maker Faire Tokyo 2026(9/4-9/8)에서 실제로
  전시했던 회로: 순정 XIAO SAMD21 위에 FSR402 1개 + LED 1개, "E. Modular"
  (FidgetSqueezer 스타일 폴딩 볼 — `hardware/cad/`와
  `hardware/docs/MFT26_devlog.pdf` 참고)를 위해 제작. 정확한 브레드보드
  배선은 해당 폴더의 README 참고. 다른 물리적 객체(20개의 독립 센싱 힌지가
  아니라 단일 squeeze-force 관절)이고 정식 연구가 아닌 특정 전시 마감을 위해
  만든 것이라 연구용 펌웨어와 분리해서 관리합니다.
- `firmware/Plato_002_fsr_led_buzzer_vibration/` — `Plato_001_fsr_led`의
  전시 이후 확장판: squeeze force 임계값을 넘으면 TMB12A05 액티브 부저(5V,
  고정 ~2.4kHz, 계속 울리지 않고 짧게 끊어지는 chirp 패턴)가 울리고, ERM
  진동모터 모듈은 누르는 힘에 비례해 세기가 연속적으로 변하는 촉감(haptic)
  레이어로 동작합니다(단순 on/off가 아니라 "울림판이 실제로 진동하는" 느낌을
  주기 위함). `Plato_001_fsr_led`는 Maker Faire에서 실제로 전시됐던 기록
  그대로 건드리지 않고, 새로운 감각 레이어는 별도의 번호가 매겨진 스케치로
  추가합니다.

## 펌웨어 (`firmware/PLATO_XIAO_SAMD21/`)

```
PLATO_XIAO_SAMD21.ino  # setup/loop, 시리얼 명령, 조건 토글링
config.h                # 핀, 멀티플렉서/타이밍/햅틱 상수, 시리얼 프레이밍
FlexMuxManager.*        # 멀티플렉서 채널 스캔, 힌지별 캘리브레이션(RAM only), fold bitmask
HapticManager.*         # non-blocking 햅틱 펄스 제어
SerialTransport.*       # 유선 USB serial 프레이밍 (임시 방식 - 아래 참고)
sketch.yaml             # arduino-cli 프로필 (보드 + 플랫폼 버전)
```

### 무선 통신은 아직 미정

SAMD21은 BLE/Wi-Fi 라디오가 없습니다. 검토했던 옵션 두 가지:

1. **유선 USB serial** (현재 구현된 방식) — 추가 하드웨어 불필요, 가장
   신뢰성 높음. 하지만 "케이블이 naturalistic-interaction 주장을 훼손하는가"
   라는, 논문 작성 시 제기됐던 우려를 다시 불러옵니다.
2. **외부 BLE UART 모듈** (예: DA14531/nRF52 기반 UART-BLE 브릿지) — 무선/
   naturalistic-use 프레이밍은 유지되지만, 노드마다 추가 하드웨어와 배선이
   필요합니다.

`SerialTransport`가 의도적으로 와이어 프로토콜을 아는 유일한 곳입니다 —
`PLATO_XIAO_SAMD21.ino`는 그냥 `transport.send(payload, len)`을 호출할 뿐입니다.
나중에 `BleUartTransport`로 교체하는 것(동일한 `send()` 시그니처, 네이티브
USB 포트 대신 BLE UART 모듈의 시리얼 패스스루로 프레이밍)은 전면 재작성이
아니라 작고 독립적인 변경이어야 합니다.

### 캘리브레이션 (힌지별 편차)

Flex 센서의 전압분배 출력은 힌지마다 다릅니다(저항 범위, 장착 프리로드).
하나의 전역 임계값 대신, 각 힌지마다 자체 캘리브레이션된 min/max를 갖습니다:

- 시리얼 모니터로 `c`를 보내면 15초짜리 캘리브레이션 창(`CALIBRATION_DURATION_MS`)이
  시작됩니다. 실행되는 동안 모든 힌지를 완전한 범위로 접었다 폈다 하세요.
- 캘리브레이션은 **RAM에만 저장**됩니다(SAMD21은 별도 라이브러리 의존성 없이는
  ESP32의 `Preferences`/NVS에 직접 대응하는 게 없음) 그리고 힌지별로 출력되니
  데이터와 함께 report할 `min/max/range` 표를 얻을 수 있습니다 — 이게 논문
  작성 시 internal-validity 우려로 지적됐던 힌지별 편차의 명시적 bounding입니다.
  RAM 전용이라서 **전원을 껐다 켤 때마다 `c`를 다시 보내야** 합니다 — 어차피
  이미 문서화된 세션별 캘리브레이션 프로토콜과 일치합니다(오래된 공장 기본값을
  믿는 대신 세션/참가자마다 한 번씩 실행).

### 조건 토글링

시리얼로 `h`를 보내면 런타임에 햅틱 피드백을 켜고 끌 수 있어서, 같은 플래시된
펌웨어가 참가자 사이에 재플래싱 없이 **Block (motor only)**와 **Block +
haptic feedback** 두 실험 조건 모두를 처리합니다. 조건 플래그는 모든 시리얼
프레임에 포함되어(아래 참고) 수동으로 추적할 필요 없이 자동으로 로깅됩니다.

### 시리얼 프레임 포맷

매 `NOTIFY_INTERVAL_MS`(기본 20Hz)마다 노드는 USB 시리얼 포트에 6바이트
프레임을 씁니다: 마커 바이트 2개(PC 쪽 리더가 프레임 중간에 연결하더라도 이
연속 스트림에서 바이트 정렬을 유지할 수 있게 하고, 가끔 발생하는 바이트
손실/손상으로부터 스스로 복구할 수 있게 함)에 이어서 ESP32C3 빌드가 BLE로
보내던 것과 동일한 4바이트 페이로드가 옵니다:

| 바이트 | 내용 |
|------|---------|
| 0    | `0xAA` 동기 바이트 1 |
| 1    | `0x55` 동기 바이트 2 |
| 2-4  | 20비트 fold bitmask, LSB부터 (bit *i* = 힌지 *i*가 folded 상태) |
| 5    | bit0: 햅틱 펄스 현재 작동 중 · bit1: 햅틱 피드백 조건 활성화됨 |

### 설정

```bash
arduino-cli config add board_manager.additional_urls \
  https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
arduino-cli core update-index
arduino-cli core install Seeeduino:samd
```

`sketch.yaml`이 보드/플랫폼 버전을 고정합니다. arduino-cli 0.35 이상이면
아래 스크립트가 쓰는 `--profile` 플래그가 자동으로 설치하고 사용합니다.
외부 라이브러리는 필요 없습니다 (Arduino SAMD 코어만 사용).

Arduino IDE에서는: Tools > Board > **Seeed SAMD Boards > Seeeduino XIAO**.

### 빌드 & 플래시

```bash
scripts/build.sh
scripts/upload.sh /dev/ttyACM0   # 보드의 실제 시리얼 포트로 바꾸세요
scripts/monitor.sh /dev/ttyACM0
```

## PC 브릿지 (`bridge/`)

노드는 유선 USB serial로 스트리밍합니다(위의 "무선 통신은 아직 미정" 참고).
브릿지가 그 포트를 직접 열어서 각 읽음값을 로컬 CSV, 선택적 git commit+push,
그리고/또는 Google Sheet로 분배합니다.

```bash
pip install -r bridge/requirements.txt

# CSV만:
python bridge/plato_bridge.py --ports /dev/ttyACM0

# CSV + 50행마다 자동 commit + Google Sheets로 push:
python bridge/plato_bridge.py \
  --ports /dev/ttyACM0 \
  --git-commit-every 50 \
  --sheets-url "https://script.google.com/macros/s/XXXX/exec"
```

- `--ports` (기본값 `/dev/ttyACM0`): 물리 노드당 하나씩, 쉼표로 구분한 시리얼
  포트들 — 각각 자기 스레드에서 실행되므로, 구조물이 여러 개면 `--ports`
  항목만 여러 개 추가하면 됩니다 (예: `/dev/ttyACM0,/dev/ttyACM1`).
- `--baud` (기본값 115200): `config.h`의 `SERIAL_BAUD`와 일치해야 합니다.
- `--csv` (기본값 `data/plato_log.csv`): 프레임당 한 행, 컬럼은
  `timestamp_utc,node,haptic_active,haptic_enabled,hinge_00..hinge_19`.
  (전환 시점만이 아니라) 모든 스냅샷을 로깅하므로 손상되거나 누락된 프레임
  하나가 이벤트를 아예 잃어버리게 만들지 않습니다 — fold 시퀀스 순서와
  힌지별 전환 타이밍(Kendall's Tau / Spearman's rho / edit-distance 분석용)은
  스트림 자체에 의존하지 않고 이후 시계열 데이터에서 재구성됩니다.
- `--git-commit-every N`: CSV를 N개의 새 행마다 commit+push (0이면 비활성화).
  브릿지가 이 저장소에 push 권한이 있는 곳에서 실행되어야 합니다.
- `--sheets-url` / `GOOGLE_SHEETS_WEBHOOK_URL`: 각 행을 JSON으로 Google Apps
  Script Web App에 POST합니다. 대상 Sheet의 Apps Script 편집기에 붙여넣고
  Web App으로 배포할 스크립트는 `bridge/google_apps_script.gs` 참고
  (Extensions → Apps Script → Deploy → New deployment).

세션 도중 노드 연결이 끊기면 브릿지를 재시작하세요 — 포트당 단순한
open-and-stream 루프이고, 아직 재연결 로직은 없습니다.

## 확장하기

- 구조물/노드 여러 개: 각각을 자기 USB 포트로 연결하고 `--ports`에 추가하면
  됩니다. 브릿지는 이미 모든 포트를 동시에 받아들입니다.
- 힌지 개수 변경: `FlexMuxManager::kChannelCount`와 멀티플렉서 배선은 그
  상수만 바꾸면 (16채널 멀티플렉서 2개로) 최대 32개까지 어떤 개수로도
  펌웨어 변경 없이 확장됩니다. CAD에서 힌지 개수를 바꿀 때는 이 상수와 반드시
  동기화하세요 (`hardware/cad/README.md` 참고).
- 무선 추가하기: `SerialTransport`와 동일한 `send(data, len)` 시그니처를 갖는
  `BleUartTransport`를 구현해서 `PLATO_XIAO_SAMD21.ino`에서 교체하면 됩니다 —
  나머지 펌웨어는 바뀔 필요가 없습니다.
