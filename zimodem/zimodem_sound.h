/*
   ===================================================================
   ZiModem Sound Engine
   Developed by Claude, refined by NSteven
   ===================================================================
   [Audio Architecture Overview]
   - Uses ESP32 FreeRTOS tasks to handle asynchronous, non-blocking
     sound playback sequences.
   - Utilizes a high-precision 16kHz hardware timer ISR to execute
     Direct Digital Synthesis (DDS) for real-time waveform generation.

   [Sound Synthesis & Features]
   - Multi-Tone: Mixes up to 4 concurrent frequencies using a pre-calculated
     256-byte sine table and phase increments for accurate DTMF/signals.
   - Modem Data Emulation: Pre-calculates phase increments outside the ISR
     to toggle frequencies efficiently without division overhead.
   - Noise Generation: Employs a Linear Congruential Generator (LCG)
     to inject high-fidelity 8-bit white noise into the audio mixer.
   - 1-Bit PWM Output: Drives a passive buzzer via a sigma-delta-like
     comparator logic (mixed > 0 ? HIGH : LOW) for a retro analog aesthetic.
   ===================================================================
*/
#pragma once
#ifdef ZIMODEM_ESP32

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ─── 설정 ────────────────────────────────────────────────
#ifndef DEFAULT_PIN_SND
#define DEFAULT_PIN_SND GPIO_NUM_6
#endif
#define SND_SAMPLE_RATE 16000 // 16kHz

// ─── 사운드 태스크 동기화 상태 ────────────────────────────
enum SndState {
  SND_IDLE,
  SND_DIALING,     // DTMF 재생 중
  SND_WAIT_RESULT, // 접속 결과 대기 중
  SND_HANDSHAKE,   // V.90 굉음 재생 지시
  SND_BUSY,        // Busy Tone 재생 지시
  SND_DONE         // 재생 완료
};

// ─── 전역 변수 ────────────────────────────────────────────
static int8_t snd_sine_table[256];
static volatile uint16_t snd_phase[4] = {0, 0, 0, 0};
static volatile uint16_t snd_phase_inc[4] = {0, 0, 0, 0};
static volatile uint8_t snd_active_tones = 0;
static volatile bool snd_tone_active = false;
static volatile bool snd_data_mode = false;
static volatile uint16_t snd_data_inc[4] = {0, 0, 0, 0};
static volatile uint16_t snd_extra_tone = 0;
static volatile uint16_t snd_extra_inc = 0;
static volatile bool snd_noise_active = false;
static volatile uint32_t snd_rand_seed = 123456789;

static esp_timer_handle_t snd_timer = NULL;
static TaskHandle_t snd_task_handle = NULL;
static volatile SndState snd_state = SND_IDLE;
static char snd_dial_buf[256]; // 다이얼 문자열 저장용

// ATM 명령으로 제어되는 스피커 모드 (zcommand.h의 speakerMode 반영)
// 0=항상끔, 1=다이얼/시도중만켬, 2=항상켬
static int snd_speaker_mode = 1;

// ─── 유틸리티 ─────────────────────────────────────────────
static inline uint16_t snd_f2p(uint16_t freq) {
  return (uint16_t)((uint32_t)freq * 65536UL / SND_SAMPLE_RATE);
}

// ─── 하드웨어 타이머 ISR (16kHz, 항상 실행) ─────────────────
static void IRAM_ATTR snd_isr(void *arg) {
  int pin = DEFAULT_PIN_SND;
  if (pin < 0)
    return;
  if (!snd_tone_active && !snd_noise_active) {
    digitalWrite(pin, LOW);
    return;
  }
  int16_t mixed = 0;
  if (snd_tone_active) {
    if (snd_data_mode) {
      static bool toggle = false;
      toggle = !toggle;
      mixed += snd_sine_table[snd_phase[0] >> 8];
      mixed += snd_sine_table[snd_phase[1] >> 8];
      snd_phase[0] += toggle ? snd_data_inc[0] : snd_data_inc[1];
      snd_phase[1] += toggle ? snd_data_inc[2] : snd_data_inc[3];
      if (snd_extra_tone > 0) {
        mixed += snd_sine_table[snd_phase[2] >> 8];
        snd_phase[2] += snd_extra_inc;
      }
    } else {
      for (int i = 0; i < snd_active_tones; i++) {
        mixed += snd_sine_table[snd_phase[i] >> 8];
        snd_phase[i] += snd_phase_inc[i];
      }
    }
  }
  if (snd_noise_active) {
    snd_rand_seed = (uint32_t)(snd_rand_seed * 1664525UL + 1013904223UL);
    mixed += (int8_t)(snd_rand_seed >> 24);
  }
  digitalWrite(pin, mixed > 0 ? HIGH : LOW);
}

// ─── 기본 재생 함수 (사운드 태스크 내부에서만 호출) ──────────
static void snd_play_multi(uint16_t f1, uint16_t f2, uint16_t f3, uint16_t f4,
                           uint32_t ms) {
  // ISR이 쓸레없는 이전 톤 끄기
  snd_tone_active = false;
  snd_noise_active = false;
  snd_data_mode = false;
  snd_extra_tone = 0;
  // 위상/증분 초기화 (tone_active 해제 상태에서)
  uint16_t freqs[4] = {f1, f2, f3, f4};
  uint8_t cnt = 0;
  for (int i = 0; i < 4; i++) {
    if (freqs[i] > 0) {
      snd_phase[cnt] = 0;
      snd_phase_inc[cnt] = snd_f2p(freqs[i]);
      cnt++;
    }
  }
  snd_active_tones = cnt;
  // 모든 데이터 준비 후에 ISR 활성화
  snd_tone_active = true;
  delay(ms);
  snd_tone_active = false;
  if (DEFAULT_PIN_SND >= 0)
    digitalWrite(DEFAULT_PIN_SND, LOW);
}

static void snd_play_modem_data(uint16_t b1, uint16_t b2, uint16_t overlay,
                                uint32_t ms) {
  // ISR 끄기 후 데이터 준비
  snd_tone_active = false;
  snd_noise_active = false;
  snd_phase[0] = snd_phase[1] = snd_phase[2] = 0;
  snd_data_inc[0] = snd_f2p(b1);
  snd_data_inc[1] = snd_f2p(b1 + 200);
  snd_data_inc[2] = snd_f2p(b2);
  snd_data_inc[3] = snd_f2p(b2 + 350);
  snd_extra_tone = overlay;
  snd_extra_inc = (overlay > 0) ? snd_f2p(overlay) : 0;
  snd_data_mode = true;
  // 모든 데이터 준비 후 ISR 활성화
  snd_tone_active = true;
  delay(ms);
  snd_tone_active = false;
  snd_data_mode = false;
  snd_extra_tone = 0;
  if (DEFAULT_PIN_SND >= 0)
    digitalWrite(DEFAULT_PIN_SND, LOW);
}

static void snd_play_noise(uint32_t ms) {
  snd_tone_active = false;
  snd_data_mode = false;
  snd_extra_tone = 0;
  snd_noise_active = true;
  delay(ms);
  snd_noise_active = false;
  if (DEFAULT_PIN_SND >= 0)
    digitalWrite(DEFAULT_PIN_SND, LOW);
}

static void snd_play_silence(uint32_t ms) {
  snd_tone_active = false;
  snd_data_mode = false;
  snd_extra_tone = 0;
  snd_noise_active = false;
  if (DEFAULT_PIN_SND >= 0)
    digitalWrite(DEFAULT_PIN_SND, LOW);
  delay(ms);
}

// ─── DTMF 한 글자 재생 (숫자 0-9, *, #, 영문자 A-Z 지원) ───────
// 영문자는 전화 키패드 표준 매핑으로 변환:
//   ABC=2, DEF=3, GHI=4, JKL=5, MNO=6, PQRS=7, TUV=8, WXYZ=9
static void snd_dtmf_digit(char c) {
  // 영문자를 키패드 숫자로 변환
  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
    char lc = (c >= 'A' && c <= 'Z') ? c + 32 : c; // 소문자로 통일
    if      (lc >= 'a' && lc <= 'c') c = '2';
    else if (lc >= 'd' && lc <= 'f') c = '3';
    else if (lc >= 'g' && lc <= 'i') c = '4';
    else if (lc >= 'j' && lc <= 'l') c = '5';
    else if (lc >= 'm' && lc <= 'o') c = '6';
    else if (lc >= 'p' && lc <= 's') c = '7';
    else if (lc >= 't' && lc <= 'v') c = '8';
    else if (lc >= 'w' && lc <= 'z') c = '9';
    else return;
  }

  uint16_t lo = 0, hi = 0;
  if (c == '1' || c == '2' || c == '3')
    lo = 697;
  else if (c == '4' || c == '5' || c == '6')
    lo = 770;
  else if (c == '7' || c == '8' || c == '9')
    lo = 852;
  else if (c == '*' || c == '0' || c == '#')
    lo = 941;

  if (c == '1' || c == '4' || c == '7' || c == '*')
    hi = 1209;
  else if (c == '2' || c == '5' || c == '8' || c == '0')
    hi = 1336;
  else if (c == '3' || c == '6' || c == '9' || c == '#')
    hi = 1477;

  if (lo == 0 || hi == 0)
    return;
  snd_play_multi(lo, hi, 0, 0, 100);
  snd_play_silence(100); // 100ms 묵음: 연속 동일 음도 명확히 구분
}

// ─── URL/주소에서 DTMF 시퀀스 재생 ──────────────────────────
// 규칙: http://, ftp:// 등 프로토콜 제거 후
//       숫자(0~9), *, # → 직접 DTMF 재생
//       영문자(A~Z, a~z) → 전화 키패드 매핑 후 DTMF 재생
//       점(.), 하이픈(-) 등 구분자 → 짧은 묵음으로 처리
static void snd_dial_sequence(const char *addr) {
  const char *p = addr;
  // 프로토콜 접두사 건너뜀 (예: "http://")
  const char *proto = strstr(addr, "://");
  if (proto != NULL)
    p = proto + 3;

  // 터미널 매크로 등으로 인해 주소 맨 앞에 ATDT/ATDP가 중복 포함되어 전달될 경우 무시
  if (strncasecmp(p, "ATDT", 4) == 0 || strncasecmp(p, "ATDP", 4) == 0 || strncasecmp(p, "ATD", 3) == 0) {
    p += (strncasecmp(p, "ATD", 3) == 0 && (p[3] == 't' || p[3] == 'T' || p[3] == 'p' || p[3] == 'P')) ? 4 : 3;
  }

  // 발신음 (0.4초)
  snd_play_multi(350, 440, 0, 0, 400);
  snd_play_silence(100);

  // DTMF 다이얼링: 숫자, *, #, 영문자 → 연속 재생 (구분자 무시)
  while (*p != '\0') {
    char c = *p++;
    if ((c >= '0' && c <= '9') || c == '*' || c == '#') {
      snd_dtmf_digit(c);  // 숫자/기호 직접 DTMF
    } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      snd_dtmf_digit(c);  // 영문자 → 키패드 매핑 DTMF
    }
    // 점(.), 콜론(:), 하이픈(-) 등 모든 구분자 완전 무시
  }
  snd_play_silence(300);
}

// ─── 링백 톤 (뚜르르릉 × 1회 + 1초 묵음) ─────────────────────
static void snd_ringback_tone() {
  snd_play_multi(440, 480, 0, 0, 1500);
  snd_play_silence(1000);
}

// ─── Busy Tone (뚜- 뚜- × 4회) ───────────────────────────
static void snd_busy_tone() {
  for (int i = 0; i < 4; i++) {
    snd_play_multi(480, 620, 0, 0, 500);
    snd_play_silence(500);
  }
}

// ─── V.90 모뎀 핸드셰이크 굉음 시퀀스 ────────────────────────
static void snd_handshake_sequence() {
  snd_play_multi(1300, 2600, 0, 0, 800);
  snd_play_silence(200);
  snd_play_multi(600, 1400, 2000, 3000, 500);
  snd_play_multi(700, 1500, 2200, 0, 500);
  snd_play_multi(1500, 1900, 0, 0, 100);
  snd_play_silence(200);

  snd_play_multi(1000, 2000, 3000, 0, 200);
  snd_play_modem_data(1150, 2300, 0, 800);
  snd_play_multi(1800, 3500, 0, 0, 200);
  snd_play_modem_data(1200, 2800, 0, 800);
  snd_play_multi(1000, 2000, 3000, 0, 200);
  snd_play_modem_data(1150, 2300, 0, 300);
  snd_play_silence(200);

  snd_play_multi(2100, 2100, 0, 0, 1200);
  snd_play_modem_data(650, 950, 2100, 800);
  snd_play_modem_data(1400, 1850, 0, 800);
  snd_play_modem_data(900, 2400, 0, 200);

  // 뛰용 스윕 1차
  uint32_t t1 = millis();
  int s1 = 0;
  while (millis() - t1 < 200) {
    uint16_t sf = 2400 - (s1 * 24);
    snd_tone_active = false;
    snd_phase_inc[0] = snd_f2p(600);
    snd_phase_inc[1] = snd_f2p(sf);
    snd_phase_inc[2] = snd_f2p(sf + 500);
    snd_active_tones = 3;
    snd_tone_active = true;
    delay(4);
    s1++;
  }
  snd_tone_active = false;

  snd_play_multi(1800, 0, 0, 0, 500);

  // 뛰용 스윕 2차
  uint32_t t2 = millis();
  int s2 = 0;
  while (millis() - t2 < 200) {
    uint16_t sf = 2400 - (s2 * 24);
    snd_tone_active = false;
    snd_phase_inc[0] = snd_f2p(600);
    snd_phase_inc[1] = snd_f2p(sf);
    snd_phase_inc[2] = snd_f2p(sf + 500);
    snd_active_tones = 3;
    snd_tone_active = true;
    delay(4);
    s2++;
  }
  snd_tone_active = false;

  snd_play_multi(1800, 0, 0, 0, 600);
  snd_play_silence(200);
  snd_play_multi(2600, 2600, 0, 0, 200);
  snd_play_multi(2600, 2680, 0, 0, 200);
  snd_play_noise(1500);
  snd_play_silence(50);
  snd_play_noise(3000);
}

// ─── FreeRTOS 사운드 태스크 ────────────────────────────────
// 이 태스크 안에서 delay()를 써도 ZiModem 메인루프는 블로킹되지 않음
static void snd_task_fn(void *param) {
  if (DEFAULT_PIN_SND >= 0) {
    // 1. DTMF 다이얼링 (snd_dial_buf에 담긴 주소로)
    snd_dial_sequence(snd_dial_buf);

    // 2. 메인 태스크에 "다이얼 완료, 접속 결과 줘" 신호
    snd_state = SND_WAIT_RESULT;

    // 3. 메인 태스크가 접속 결과를 알려줄 때까지 대기 (최대 30초 타임아웃)
    uint32_t wait_start = millis();
    while (snd_state == SND_WAIT_RESULT) {
      if (millis() - wait_start > 30000) { // 30초 타임아웃
        snd_state = SND_DONE;
        break;
      }
      delay(10);
    }

    // 4. 결과에 따라 재생
    if (snd_state == SND_HANDSHAKE) {
      snd_ringback_tone();
      snd_handshake_sequence();
    } else if (snd_state == SND_BUSY) {
      snd_busy_tone();
    }
  }

  snd_state = SND_DONE;
  snd_task_handle = NULL;
  vTaskDelete(NULL);
}

// ─── 공개 API ─────────────────────────────────────────────

// setup()에서 한 번 호출
static void snd_init() {
  // 사인 테이블 초기화
  for (int i = 0; i < 256; i++)
    snd_sine_table[i] = (int8_t)(127.0f * sinf(2.0f * M_PI * i / 256.0f));

  int pin = DEFAULT_PIN_SND;
  if (pin < 0)
    return;

  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);

  // 하드웨어 타이머 ISR 시작 (16kHz)
  const esp_timer_create_args_t args = {.callback = &snd_isr,
                                        .arg = NULL,
                                        .dispatch_method = ESP_TIMER_TASK,
                                        .name = "snd_isr",
                                        .skip_unhandled_events = true};
  ESP_ERROR_CHECK(esp_timer_create(&args, &snd_timer));
  ESP_ERROR_CHECK(
      esp_timer_start_periodic(snd_timer, 1000000 / SND_SAMPLE_RATE));
}

// doDialStreamCommand()에서 접속 전에 호출.
// 사운드 태스크를 시작하고 즉시 반환(비동기).
static void snd_start_dial(const char *addr) {
  if (snd_speaker_mode == 0 || DEFAULT_PIN_SND < 0)
    return;
  if (snd_task_handle != NULL)
    return; // 이미 재생 중이면 무시

  strncpy(snd_dial_buf, addr ? addr : "", sizeof(snd_dial_buf) - 1);
  snd_dial_buf[sizeof(snd_dial_buf) - 1] = '\0';
  snd_state = SND_DIALING;

  xTaskCreate(snd_task_fn, "snd_task", 4096, NULL, 1, &snd_task_handle);
}

// doDialStreamCommand()에서 TCP 접속 결과가 나온 후 호출.
// 사운드 태스크가 결과에 따라 굉음 or Busy Tone을 재생.
// 사운드가 완전히 끝날 때까지 대기한 후 반환.
static void snd_finish(bool connected) {
  if (snd_speaker_mode == 0 || DEFAULT_PIN_SND < 0)
    return;

  // DTMF가 끝나고 SND_WAIT_RESULT 상태가 될 때까지 대기
  uint32_t timeout = millis();
  while (snd_state == SND_DIALING && (millis() - timeout < 30000))
    delay(10);

  // 접속 결과 알림
  if (snd_state == SND_WAIT_RESULT)
    snd_state = connected ? SND_HANDSHAKE : SND_BUSY;

  // 재생 완전히 끝날 때까지 대기 (굉음 ~15초, Busy Tone ~4초)
  timeout = millis();
  while (snd_state != SND_DONE && snd_state != SND_IDLE &&
         (millis() - timeout < 30000))
    delay(10);
}

#endif // ZIMODEM_ESP32
