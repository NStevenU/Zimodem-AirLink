<p align="center">
  <img src="AirLink Logo.svg" width="600" alt="AirLink Serial 56K Logo">
</p>

<p align="center">
  <strong>ESP32-C3 기반 IBM PC 전용 고품질 WiFi 모뎀 에뮬레이터 (한국어 지원!)</strong><br>
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

## 🛠️ Hardware Requirements
* **Microcontroller**: ESP32-C3
* **Serial Communication**: MAX3232 (RS232 to TTL)
* **Audio**: Buzzer (for Sound Emulation)

## 📜 Credits & License
* **Developer**: NSteven, Republic of Korea ([GitHub](https://github.com/NStevenU/))
* **Original Firmware**: Zimodem 4.0 (C)2016-2026 Bo Zimmerman ([GitHub](https://github.com/bozimmerman/Zimodem))
* **AI Assistance**: 
  * **Claude 4.6 Sonnet (Thinking)**: 사운드 엔진(DDS) 기초 아키텍처 설계 및 주요 로직 최적화 
  * **Gemini 3.1 Pro (High)**: 코드 리뷰, 한글 인코딩(EUC-KR) 변환 및 문서화 지원
* **License**: [Apache License 2.0](LICENSE)
