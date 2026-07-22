<p align="center">
  <img src="AirLink Logo.svg" width="600" alt="AirLink Serial 56K Logo">
</p>

<p align="center">
  <strong>ESP32-C3 기반 IBM PC Serial - WiFi 모뎀 에뮬레이터 (한국어 지원!)</strong><br>
  <em>Based on <a href="https://github.com/bozimmerman/Zimodem">Zimodem 4.0</a> by Bo Zimmerman</em>
</p>

---

## 📌 Overview
**AirLink 56K**는 Zimodem 펌웨어를 기반으로 최적화와 수정, 개선을 거친 WiFi 모뎀 펌웨어입니다. 레트로 IBM PC 환경에 완벽하게 호환되도록 설계되었으며, **ESP32-C3** 칩셋의 성능을 활용하여 실제 모뎀의 감성적인 사운드와 현대적인 편의성(VT100 메뉴, 한글 지원 등)을 동시에 제공합니다.
(AirLink는 실제로 존재하는 브랜드가 아니며 그냥 어울리도록 만든 것입니다. 누구나 따라 사용하실 수 있습니다.)

> 🇰🇷 **Note for International Users:** 
> This firmware is highly optimized for the Korean environment. While English is fully supported, please note that Korean is set as the default language.

**중요한 점**으로 이 펌웨어는 단순히 개인적인 용도로 프로젝트를 만들어 만든 것이기 때문에 최종 완성본이 아니며, 각종 문제 또는 미숙한 부분, 버그등이 발생할 수 있습니다.
또한 수정되거나 추가된 기능 대비 설명이 부족한 부분도 있습니다.

## ✨ Key Modifications & Features
* **최신 개발 환경 대응**: 최신 Arduino IDE, ESP-IDF v5.0 이상 대응
* **AirLink 브랜드 적용**: ESP32-C3, MAX3232, Buzzer를 결합한 하드웨어에 사용할 브랜드 적용(그래야지 진짜 물건같고 있어보이잖아요?ㅎㅎ)
* **IBM PC 전용 최적화**: 범용 레트로 PC 지원 코드를 덜어내고, ESP32-C3에 맞지 않는 불필요한 명령어 비활성화
* **모뎀 사운드 에뮬레이션!!!**: 
  * 전화 벨소리(Ringing), DTMF 다이얼 톤, V.90 모뎀 접속음을 추가 및 유기적 연동
  * 실제 문자열 접속 주소 입력 시, 사운드 출력을 위해 DTMF 음으로 자동 대체하여 레트로 감성 극대화
* **VT100 대화형 설정 메뉴**: 터미널에서 직관적으로 모뎀을 세팅할 수 있는 `AT+CONFIG` 메뉴 전면 개편
* **완벽한 한국어(EUC-KR) 지원**: 설정 메뉴 및 출력 메시지에서 영어/한국어 선택 기능 추가
* **사용자 편의성 강화**: 자체 도움말 기능(`AT+HELP`) 및 기타 마이너 편의 기능 탑재
* **한국 맞춤 최적화**: 인터넷 연결 기능인 SLIP, PPP기능의 가상번호를 PC통신 느낌이 나도록 014XY로 바꾸어, 01480, 01481로 수정
* **완벽한 Windows 9X PPP 호환성**: 
  * 기존 펌웨어에 없던 PAP(Password Authentication Protocol) 인증 로직을 자체적으로 완전 신규 구현하여 "사용자 이름 및 암호 확인 중" 화면에서 멈추는 고질적 문제 해결
  * IPCP(IP Control Protocol) 교섭 과정에서 모뎀과 윈도우 간의 IP 할당 충돌(Conflict) 방지 알고리즘 적용 및 NAK/REJ 핸들링 추가
* **레트로 모뎀 속도 완벽 에뮬레이션 (QoS 대역폭 제한)**:
  * Token Bucket 알고리즘을 도입하여 56K, 33.6K, 28.8K, 14.4K, 9.6K, 2.4K 등 과거 전화선 모뎀 특유의 병목 현상과 느린 전송 속도를 완벽하게 재현 (PPP, SLIP, 일반 Stream 모드 전체에 일괄 적용, 물론 무제한(0)으로 설정하여 최고속도로 사용가능)
* **상태 표시(Status) LED 기능 활성화**:
  * ESP32-C3 하드웨어 핀 맵핑을 전면 재정비하고, 직관적인 상태 파악을 위한 3개의 LED(PWR 전원, WIFI 연결 상태, RX/TX 데이터 송수신) 깜빡임 기능 추가
* **USB 비상 디버그 콘솔 (Emergency Backdoor)**:
  * 본선 시리얼 터미널이 먹통이 되었을 때 칩의 USB 포트(115200bps)로 직접 연결하여 하드웨어 강제 재부팅(`RESET`), 속도 강제 복구(`BAUD=`), 공장 초기화(`FACTORY`) 등을 수행할 수 있는 비상 커맨드 시스템 구축

## 🔌 Hardware Configuration (AirLink ESP32-C3 Pinout)

기존 범용 ESP32에 맞춰져 있던 핀 배열을 AirLink 하드웨어(ESP32-C3)에 최적화하여 새롭게 맵핑했습니다.<br/>모뎀 터미널 프로그램 설정 시 아래의 기본값을 참고하세요.

### 1. 시리얼 통신 기본 설정 (Default Serial Settings)
* **기본 모뎀 통신 포트 (UART0)**
  * **기본 속도 (Baud Rate)**: `115200 bps` 
  * **통신 포맷**: `8-N-1` (8 Data bits, No Parity, 1 Stop bit)
  * **흐름 제어 (Flow Control)**: `소프트웨어 방식 (XON/XOFF)`이 기본으로 활성화되어 있으며, 필요시 `AT&K` 명령어, 또는 `AT+CONFIG` 메뉴에서 하드웨어(RTS/CTS) 제어로 변경할 수 있습니다.
* **USB 비상 디버그 포트 (USB CDC)**
  * 기기를 컴퓨터에 직접 USB로 연결하면 **비상 콘솔(115200 bps)** 에 접속할 수 있습니다. 메인 통신이 설정 오류로 먹통이 될 경우, 이 포트를 통해 `BAUD=`, `FACTORY`, `RESET` 등의 복구 명령을 내릴 수 있습니다.

### 2. ESP32-C3 핀 맵핑 (Pin Map)
사용자 커스텀 보드를 제작하거나 모듈을 직접 배선할 경우 아래의 핀 배열을 준수해야 합니다.

| 기능 (Function) | 설명 (Description) | GPIO 번호 (ESP32-C3) |
|:---:|---|:---:|
| **TXD** | UART Transmit Data (PC 수신) | `기본 UART0 TX (GPIO 21)` |
| **RXD** | UART Receive Data (PC 송신) | `기본 UART0 RX (GPIO 20)` |
| **DCD** | Data Carrier Detect | `GPIO 8` |
| **DTR** | Data Terminal Ready | `GPIO 5` |
| **DSR** | Data Set Ready | `GPIO 4` |
| **RTS** | Request To Send | `GPIO 2` |
| **CTS** | Clear To Send | `GPIO 3` |
| **RI** | Ring Indicator | `GPIO 1` |
| **SND** | 오디오 출력 (Buzzer/Speaker) | `GPIO 6` |
| **WIFI LED** | WiFi 연결 상태 표시 (Active LOW) | `GPIO 10` |
| **RXTX LED** | 데이터 송수신 표시 (Active LOW) | `GPIO 9` |
| **PWR LED** | 전원 표시 | 하드웨어 직결 권장 |
| **AA/OTH** | Auto Answer / Pulse Dial 핀 | `GPIO 0` / `GPIO 7` |

### 3. 하드웨어 자재 및 제작 레퍼런스 (Hardware Build & Reference)
자체 제작 시 사용된 주요 자재와 만능기판 배선, 조립 및 완제품 사진 레퍼런스입니다.<br/>
3D 프린팅 케이스는 3d case폴더 내에 stl파일로 존재하니 다운받아 출력하시면 됩니다.

#### 📦 주요 사용 자재
* **MCU**: ESP32-C3 Super Mini
* **시리얼 변환기**: MAX3232 (RS232 to TTL)
* **오디오**: 수동 부저 (Buzzer)
* **상태 표시등**: 5파이 LED 3개 (PWR, RX/TX, WIFI)
* **저항**: SMD 1206사이즈 560옴 저항 3개

#### 📐 회로 설계 & 만능기판 배선
만능기판을 활용한 핀 배선 및 설계 레이아웃 예시입니다.<br/>
![회로 설계](image/23.jpg)

#### 🔌 실제 만능기판 실장
실제 만능기판 부품 배치 및 납땜 실장 모습입니다.<br/>
![만능기판 실장 1](image/24.jpg)<br/>
![만능기판 실장 2](image/25.jpg)

#### 📦 케이스 및 내부 조립
케이스 외관 및 내부 조립 배치 사진입니다.<br/>
![케이스 사진](image/26.jpg)<br/>
![조립 내부 사진](image/27.jpg)

#### 🎨 전체 디자인 및 외관
완성된 AirLink 56K 모뎀의 디자인 모습입니다.<br/>
![전체 디자인 1](image/28.jpg)<br/>
![전체 디자인 2](image/29.jpg)<br/>
![전체 디자인 3](image/30.jpg)<br/>
![전체 디자인 4](image/31.jpg)<br/>
![전체 디자인 5](image/32.jpg)

#### 💡 작동 및 LED 동작
전원 및 데이터 송수신(RXTX), WiFi 상태 LED 동작 모습입니다.<br/>
![작동 및 LED 동작](image/33.jpg)

## 📖 사용 방법 (How to Use)
**※ 본 가이드는 Windows 9X (사진은 Windows 95) 환경을 기준으로 작성되었습니다.**

### 1. 하드웨어 연결 및 포트 기본 설정
모뎀을 사용하기 전, 올바른 순서로 연결하고 통신 포트를 설정해야 합니다.

1. **하드웨어 연결**: 먼저 AirLink 보드의 **USB-C 포트에 전원을 연결**한 후, **그 뒤에 PC와 시리얼 케이블을 연결**합니다.
2. **장치 관리자 포트 설정**: 장치 관리자로 들어갑니다.<br/>
   ![장치 관리자](image/1.jpg)
3. 연결된 **COM 포트의 등록 정보(속성)** 를 엽니다.
4. **포트 설정** 탭에서 **초당 비트 수를 `115200`** 으로, **흐름 제어를 `Xon / Xoff` (소프트웨어)** 로 변경하고 확인을 누릅니다.<br/>
   ![포트 설정](image/2.jpg)

### 2. Windows 9X 모뎀 등록 방법
이제 설정한 통신 포트에 표준 모뎀을 등록해야 합니다.

1. **제어판 -> 모뎀**을 실행하고 **`다음`** 을 누릅니다.<br/>
   ![모뎀을 검색할려고 합니다](image/3.jpg)
2. 자동으로 모뎀을 검색하게 두었다가, 만약 "모뎀을 찾을 수 없습니다"라고 나오면 **`다음`** 을 누릅니다.<br/>
   ![모뎀을 찾을 수 없습니다](image/4.jpg)
3. 제조업체 목록에서 **`표준 모뎀 형식`** 을 고르고, 모델에서 **`표준 28800 bps 모뎀`** (목록에 보이는 것 중 제일 높은 속도)을 선택합니다.<br/>
   ![모뎀의 제조업체와 모델을 선택하십시오](image/5.jpg)
4. 시리얼 케이블이 연결된 **포트를 선택**하고 **`다음`** 을 눌러 완료합니다.<br/>
   ![이 모뎀에 사용할 포트를 선택하십시오](image/6.jpg)
   <br/>
   ![모뎀이 설치되었습니다](image/7.jpg)
6. 모뎀 목록 창으로 돌아와서, 방금 등록된 모뎀을 선택하고 **`등록 정보`** 를 누릅니다.<br/>
   ![모뎀 등록 정보](image/8.jpg)
7. 일반 탭에서 **최대 속도를 `115200`** 으로 변경합니다.<br/>
   ![최대 속도](image/9.jpg)
8. **`연결`** 탭으로 이동하여 **`고급`** 버튼을 누릅니다.<br/>
   ![고굽](image/10.jpg)
9. 고급 연결 설정에서 **흐름 제어 사용**이 체크되어있는지 확인 후 **`소프트웨어 (XON/XOFF)`** 로 선택한 뒤 확인을 누릅니다.<br/>
   ![고급 옵션 설정](image/11.jpg)

### 3. 초기 부팅 후 WiFi 설정
1. 모뎀이 준비되었다면, 터미널 프로그램(예: 새롬데이타맨, 이야기 등)을 엽니다.<br/>
   ![새롬 데이타맨 98](image/12.jpg)
1. 터미널에서 `AT+CONFIG`를 입력하여 대화형 설정 메뉴로 진입합니다.
2. 메뉴에서 `w`를 입력하여 WiFi 설정으로 들어갑니다.<br/>
   ![AT+CONFIG](image/13.jpg)
3. SSID 네트워크 목록 번호를 입력하고, 공유기 비밀번호를 입력합니다.
4. DHCP 설정 화면이 나타나면, `d`를 입력하여 **DHCP 사용**으로 전환(Toggle)한 뒤 엔터를 누릅니다.<br/>
   ![WIFI 설정](image/14.jpg)
5. 연결 성공시 `와이파이 상태:`에 SSID가 표시됩니다. `엔터`를 눌러 저장하고, `y`를 눌러 저장합니다.<br/>
   ![최종 설정](image/15.jpg)

이제 모든 준비가 끝났습니다!

### 4. BBS 접속하기
아주 쉽습니다. `ATDT"원하는 주소"`를 입력하면 됩니다.<br/>
![BBS접속](image/16.jpg)

### 5. PPP 인터넷 접속하기 (01481)
다이얼업 네트워킹을 통해 TCP/IP 웹서핑에 접속합니다.

#### [전화 접속 네트워킹 설정]
1. **새 연결 만들기**: `전화 접속 네트워킹`에서 새로 연결을 엽니다.
2. 사용할 모뎀을 앞서 만든 `표준 28800 bps 모뎀`으로 선택하고, 전화번호에 **`01481`** 을 입력합니다.
3. 생성된 아이콘을 우클릭하여 `속성`에 들어갑니다. 국가/지역 번호 사용을 **해제**합니다.<br/>
   ![01481 일반](image/17.jpg)
4. `서버 종류` 탭으로 들어갑니다.
5. **`네트워크에 로그온`, `소프트웨어 압축 가능`, `암호화`** 를 끕니다.
6. 지원되는 네트워크 프로토콜에서 **NetBEUI, IPX/SPX 호환 체크 해제**, 오직 **TCP/IP**만 체크합니다.<br/>
   ![01481 서버 종류](image/18.jpg)

#### [인터넷 접속!]
1. 연결 아이콘을 더블클릭합니다.
2. 접속 창이 뜨면 사용자 이름에 **`airlink`**, 암호에 **`1111`** 을 입력합니다.<br/>
   ![연결할 대상](image/19.jpg)
3. `연결` 버튼을 누르면 인터넷이 연결됩니다.<br/>
   ![연결 설정 완료](image/20.jpg)
   <br/>
   ![IE](image/21.jpg)

### 6. 기타 설정하기
터미널에서 `AT+CONFIG`를 입력하여 대화형 설정 메뉴로 진입합니다.<br/>
여기서 바로 **`흐름 제어`**, **`언어`**, **`포트 속도`**, **`스피커`** 등을 설정 가능합니다.<br/>
예시로 모뎀 접속음이 시끄럽고, 딜레이 없이 바로 진입하고 싶으면 **`s`** 를 눌러 스피커를 음소거로 설정하면 됩니다.<br/>
![AT+CONFIG](image/22.jpg)

## 🎵 Audio Engine Architecture

오리지널 Zimodem 펌웨어에는 사운드 출력 기능이 아예 존재하지 않았습니다. 본 프로젝트는 사운드 기능이 전무했던 기존 코드 베이스에 **소프트웨어 기반 오디오 합성 엔진(DDS)을 새롭게 구현하고 모뎀 통신 시퀀스와 완벽하게 연동**하여, 실제 56K 모뎀의 아날로그 감성을 재현해 냈습니다.

### 1. 기술적 특징 (Technical Highlights)
* **하드웨어 타이머 기반 DDS (Direct Digital Synthesis)**
  * 16kHz 주기로 동작하는 고정밀 하드웨어 타이머 ISR(인터럽트 서비스 루틴)을 사용하여 실시간으로 오디오 파형을 생성합니다.
  * 미리 계산된 256-byte 사인파 테이블(Sine Table)과 위상 누적(Phase Accumulation) 기법을 사용하여, 나눗셈 연산 오버헤드 없이 빠르고 정확한 주파수를 합성합니다.
* **비동기식 FreeRTOS 태스크 연동**
  * 사운드 재생 시 `delay()`가 발생하더라도 모뎀의 메인 통신 루프가 블로킹(멈춤)되지 않도록, 사운드 처리를 전용 FreeRTOS 태스크로 분리하여 비동기식으로 실행되도록 연동했습니다.
* **다중 주파수 믹싱 및 1-Bit PWM 출력**
  * 최대 4개의 주파수(Tone)와 1개의 화이트 노이즈를 동시에 믹싱합니다.
  * 믹싱된 디지털 값을 시그마-델타(Sigma-Delta) 방식과 유사한 비교기 로직(Mixed > 0 ? HIGH : LOW)을 거쳐 1-Bit 출력으로 변환하며, 이를 통해 수동 부저(Passive Buzzer)에서도 레트로하고 아날로그적인 질감의 소리를 냅니다.

### 2. 주요 사운드 에뮬레이션 기능
* **지능형 DTMF 다이얼링 변환**
  * 사용자가 입력한 접속 주소(URL, IP 등)에서 프로토콜과 특수기호를 필터링하고, 영문자는 실제 전화 키패드 표준 매핑(예: A,B,C -> 2)에 따라 DTMF 다이얼 톤(투-티-티-티-)으로 실시간 변환하여 재생합니다.
* **V.90 모뎀 핸드셰이크 시퀀스 재현**
  * 접속 성공 시 과거 전화선 모뎀 특유의 굉음(Handshake Sound)을 재생합니다. 
  * 선형 합동 생성기(LCG) 알고리즘을 이용한 고음질 8-bit 화이트 노이즈, 주파수 스윕(Sweep), 데이터 통신 시뮬레이션 패턴을 복합적으로 조합하여 사실감을 극대화했습니다.
* **상태 기반 사운드 피드백**
  * 통신 상태(State Machine)에 따라 유기적으로 사운드가 변화합니다. (다이얼링 중 ➡️ 링백 톤 대기 ➡️ 연결 성공 시 핸드셰이크 굉음 OR 실패 시 통화 중(Busy) 톤 재생)

## 🛠️ Detailed Changelog (원본 Zimodem 대비 주요 수정 파일)
* `zimodem_sound.h` **(신규)**: 1-Bit PWM 기반 하드웨어 타이머 오디오 합성(DDS) 엔진.
* `pet2asc.h`: ESP32-C3 하드웨어 핀 맵핑 최적화, USB CDC 디버그 시리얼 통신(`DBSerial`) 정의, EUC-KR 한글 인코딩 변환 로직 추가.
* `zimodem.ino`: 메인 루프에 FreeRTOS 기반 사운드 재생 제어 연동, RX/TX/WIFI 상태 LED 점등 제어 추가, USB 비상 백도어 터미널 훅(Hook) 탑재.
* `zcommand.h` / `zcommand.ino`: AT 명령어 파서 전면 수정 (`AT+QOS`, `AT+CONFIG` 등), 한국어 번역 텍스트 적용, 모뎀 발신 시 사운드 트리거 로직 적용.
* `zconfigmode.h` / `zconfigmode.ino`: 대화형 설정 UI(`AT+CONFIG`)를 VT100 기반으로 재설계, 단축키 UI/UX 개편, 대역폭(QoS) 및 사운드 설정 메뉴 추가.
* `zpppmode.h` / `zpppmode.ino`: Windows 95 PAP 접속 인증(Authentication) 구현, 모뎀(Server)과 클라이언트(Win95) 간의 IP 할당 프로세스 규격화 및 IPCP 충돌 방지, Token Bucket 알고리즘을 이용한 QoS 속도 제한(Throttling) 구현.
* `zslipmode.ino`: SLIP 모드 연결 시 Token Bucket 기반 QoS 속도 제한 구현.
* `zstream.ino`: 일반 ATDT 통신 모드(BBS) 시에도 Token Bucket 기반 속도 제한(QoS) 적용.

## 🛠️ Hardware Requirements
* **Microcontroller**: ESP32-C3 Super Mini
* **Serial Communication**: MAX3232 (RS232 to TTL)
* **Audio**: Buzzer (for Sound Emulation)
* **Status LEDs**: 5mm LED 3개 (PWR, RXTX, WIFI)

## 📜 Credits & License
* **Developer**: NSteven, Republic of Korea ([GitHub](https://github.com/NStevenU/))
* **Original Firmware**: Zimodem 4.0 (C)2016-2026 Bo Zimmerman ([GitHub](https://github.com/bozimmerman/Zimodem))
* **AI Assistance**: 
  * **Claude 4.6 Sonnet (Thinking)**: 사운드 엔진(DDS) 기초 아키텍처 설계 및 주요 로직 최적화 
  * **Gemini 3.1 Pro (High)**: 코드 리뷰, Windows 9X 인증 버그 수정, 핀 배열 및 통신 최적화, 한글 인코딩 및 문서화 지원
* **License**: [Apache License 2.0](LICENSE)

