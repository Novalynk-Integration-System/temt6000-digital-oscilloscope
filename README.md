# TEMT6000 ESP32 Digital Oscilloscope

POC pembacaan sensor cahaya TEMT6000 dengan ESP-IDF. Saat ini firmware berada dalam
mode audit sementara: satu pembacaan ADC setiap 500 ms (2 sampel/detik). Task sampling dan
task serial berjalan terpisah. Setiap sampel masuk ke FreeRTOS queue, kemudian task
serial mengirimkannya ke aplikasi Python yang menampilkan grafik real-time.

## Hardware awal

- ESP32-C6 Mini
- Modul TEMT6000
- `VCC` TEMT6000 ke `3V3`
- `GND` TEMT6000 ke `GND`
- `SIG` TEMT6000 ke `GPIO0` (`ADC1_CH0`)

Jangan memberi tegangan lebih dari 3,3 V ke pin ADC. Jika GPIO0 tidak tersedia pada
board yang digunakan, ubah `kAdcChannel` di `main/config/app_config.hpp` sesuai pin
ADC board. Saat migrasi ke ESP32 klasik, pilih channel ADC1 agar tidak berkonflik
dengan Wi-Fi.

## Alur data

`TEMT6000 -> ADC task (1 ms) -> FreeRTOS queue -> serial task -> USB serial -> Python`

Frame serial berbentuk CSV:

```text
D,<sequence>,<timestamp_us>,<raw>,<millivolts>
```

Sequence dipakai Python untuk mendeteksi kehilangan frame. Queue berisi 4096 sampel
dan producer menggunakan blocking send, sehingga firmware tidak membuang sampel
secara diam-diam. Perlu dicatat: tidak ada buffer terbatas yang dapat menjamin data
selamanya bila laptop berhenti membaca; saat queue penuh sampling akan menerima
backpressure dan berhenti sampai serial kembali menguras queue.

## Build dan flash

Butuh ESP-IDF 5.x yang sudah diaktifkan pada terminal.

```powershell
idf.py set-target esp32c6
idf.py build
idf.py -p COM7 flash
```

Jangan menjalankan `idf.py monitor` bersamaan dengan aplikasi Python karena satu COM
port tidak dapat dibuka oleh dua program sekaligus.

## Menjalankan osiloskop Python

```powershell
cd python-scope
py -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
python oscilloscope.py --port COM7
```

Grafik menampilkan lima detik terakhir secara default. Ubah dengan `--window 10`.
Status bar menampilkan receive rate satu detik, rata-rata sejak startup, sequence yang
hilang, frame malformed, dan jumlah reset sequence ESP32. Serial task menguras queue
dalam batch hingga 32 sampel dan mengirim satu buffer melalui USB Serial/JTAG native
ESP32-C6 agar overhead per sampel tetap rendah pada 1 kHz. Parameter baud pada aplikasi
Python diabaikan oleh USB native, tetapi dipertahankan agar CLI tetap konsisten.

Untuk melihat osilasi saat POC, arahkan sumber cahaya yang dimodulasi (misalnya LED
yang dikendalikan PWM) ke TEMT6000. Cahaya ruangan yang stabil hanya menghasilkan
garis relatif datar; lampu AC dapat memperlihatkan ripple 100 Hz atau 120 Hz,
bergantung frekuensi listrik dan driver lampu.
