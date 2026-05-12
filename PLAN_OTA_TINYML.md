# Plan chi tiết để làm tiếp theo 2 task

File này viết đủ chi tiết để có thể mở chat mới rồi bám theo từng task mà làm tiếp.
Mục tiêu là sau phần Rule Chain hiện tại, hệ thống được mở rộng theo 2 hướng rõ ràng:

1. **Task 1 — OTA Update qua Web**
2. **Task 2 — Auto-Retrain TinyML**

Hai task này có liên quan nhau, nhưng vẫn nên tách riêng để dễ làm, dễ test, dễ chia nhánh, dễ demo.

---

# Tổng quan quan hệ giữa 2 task

## Task 1 là gì?
Task 1 xây **đường cập nhật từ xa** cho ESP32 qua WiFi.

Nói ngắn:
- build firmware mới
- vào web OTA
- upload firmware
- ESP32 reboot và chạy bản mới

## Task 2 là gì?
Task 2 xây **vòng đời model TinyML**:
- gửi anomaly lên cloud
- thu dữ liệu lại
- train model mới trên PC
- đưa model mới quay lại ESP32

## Vì sao 2 task liên quan nhau?
Vì ở giai đoạn đầu của Task 2, cách đơn giản nhất để đưa model mới về ESP32 là:
1. build model mới vào firmware
2. dùng OTA Web của Task 1 để update firmware

Nói ngắn:
- Task 1 = **kênh vận chuyển**
- Task 2 = **thứ cần huấn luyện và vận chuyển**

---

# Task 1 — OTA Update qua Web

## Mục tiêu cuối cùng
Có thể cập nhật firmware cho ESP32 qua WiFi bằng giao diện web, không cần cắm dây USB mỗi lần sửa code.

## Phạm vi task

### Trong scope
- Kiểm tra và kích hoạt `ElegantOTA` cho web server hiện tại.
- Đảm bảo sensor board dùng được OTA qua web.
- Nếu ổn, actuator board cũng có thể dùng chung cách này.
- Có quy trình test OTA rõ ràng.
- Sau update, firmware mới chạy thật và có cách kiểm tra version.

### Ngoài scope
- OTA qua internet public.
- OTA theo manifest cloud.
- OTA model độc lập.
- Rollback nâng cao.
- Update hàng loạt nhiều device tự động.

---

## Mục tiêu kỹ thuật chi tiết
Sau khi làm xong Task 1, bạn cần đạt được các điểm sau:

1. Board đang chạy có web server OTA truy cập được.
2. Có thể upload file `.bin` qua web.
3. Board reboot sau OTA.
4. Có cách xác nhận bản firmware mới đã được chạy.
5. Quy trình đủ ổn định để dùng cho demo và cho các task sau.

---

## Thiết kế khuyên dùng

### Vai trò board
- **Sensor board**: nên làm OTA trước, vì đây là board chính đang chạy nhiều chức năng.
- **Actuator board**: có thể làm sau, hoặc giữ cùng cơ chế nếu cần.

### Kiến trúc OTA giai đoạn đầu
- OTA là **local web OTA trong LAN/WiFi**.
- Người dùng truy cập IP của board trên mạng nội bộ.
- Dùng `ElegantOTA` để upload firmware.
- Không cần thêm server ngoài ở phase đầu.

---

## Checklist triển khai Task 1

## Bước 1 — Rà lại code web hiện tại
Mục tiêu: xác nhận đường web server và ElegantOTA hiện đang nằm ở đâu.

### Cần kiểm tra
- File nào start web server.
- File nào gọi `ElegantOTA.begin(...)`.
- Web server có chạy cả khi board đang ở AP mode, STA mode, hoặc AP+STA mode không.

### Kết quả mong muốn
- Biết chắc web OTA hiện đang attach vào server nào.
- Biết đường truy cập OTA khi board online.

---

## Bước 2 — Chuẩn hóa cách truy cập OTA
Mục tiêu: xác định người dùng sẽ vào OTA bằng cách nào.

### Cần chốt
- Nếu sensor board vào WiFi router, OTA truy cập bằng **Station IP**.
- Nếu board ở AP mode, OTA truy cập bằng **AP IP**.
- Nếu demo dùng mạng nội bộ, ưu tiên Station IP để ổn định hơn.

### Kết quả mong muốn
- Có 1 cách truy cập OTA cố định, dễ giải thích.

---

## Bước 3 — Thêm version firmware
Mục tiêu: sau OTA biết chắc board đã chạy bản mới.

### Nên có
- `firmware_version` hardcode theo chuỗi, ví dụ:
  - `fw_sensor_v1`
  - `fw_sensor_v2`
- In version ra Serial lúc boot.
- Nếu được, gửi version lên cloud hoặc hiển thị ở web.

### Kết quả mong muốn
- Sau OTA nhìn Serial hoặc dashboard biết ngay đã update thành công.

---

## Bước 4 — Test OTA thật
Mục tiêu: chứng minh OTA chạy end-to-end.

### Quy trình test chuẩn
1. Build firmware bản A.
2. Nạp bản A qua USB.
3. Xác nhận board chạy bản A.
4. Sửa `firmware_version` thành bản B.
5. Build file `.bin`.
6. Truy cập web OTA.
7. Upload file `.bin`.
8. Board reboot.
9. Mở monitor hoặc cloud để xác nhận board chạy bản B.

### Điều cần ghi nhận
- OTA mất bao lâu.
- Có bị treo/reboot lỗi không.
- Có cần tắt monitor COM trước khi OTA không.

---

## Bước 5 — Chuẩn hóa cách demo
Mục tiêu: để mai hoặc lúc báo cáo chỉ làm theo script ngắn.

### Script demo nên có
1. Mở web OTA.
2. Chọn firmware file.
3. Upload.
4. Chờ reboot.
5. Xem version mới.

### Deliverable của Task 1
- Web OTA chạy được.
- Có version để kiểm chứng.
- Có quy trình demo ngắn, lặp lại được.

---

## Rủi ro của Task 1
- Web server chạy nhưng OTA route không mở đúng.
- File `.bin` build sai env.
- Upload xong nhưng reboot không lên vì firmware lỗi.
- Board đổi IP sau khi reboot nên khó truy cập lại.
- Cắm monitor COM chiếm cổng trong lúc làm vài thao tác phụ.

---

## Tiêu chí hoàn thành Task 1
Task 1 được coi là xong khi:
- OTA qua web chạy được ít nhất 2 lần liên tiếp.
- Có cách xác nhận version mới.
- Sensor board update được không cần cắm dây.
- Nếu cần, actuator board cũng áp dụng được cùng cơ chế.

---

## Nhánh git đề xuất cho Task 1
- `feature/web-ota`

---

# Task 2 — Auto-Retrain TinyML

## Mục tiêu cuối cùng
Tạo vòng đời cải tiến model TinyML từ dữ liệu thật:
1. phát hiện anomaly ở edge
2. gửi dữ liệu lên cloud
3. export dữ liệu về PC
4. train model mới
5. đưa model mới về ESP32

---

## Phạm vi task

### Trong scope
- Gửi anomaly data lên cloud.
- Gắn `model_version` vào telemetry.
- Có cách export dữ liệu về PC.
- Có script retrain model trên PC.
- Có cách đưa model mới về ESP32.

### Ngoài scope ở phase đầu
- Auto-retrain full tự động không có người kiểm tra.
- OTA model độc lập hoàn chỉnh ngay từ đầu.
- A/B testing model phức tạp.
- Rollback model phức tạp.

---

## Tư duy chia nhỏ Task 2
Task 2 nên làm theo 4 phase nội bộ.

---

## Phase 2.1 — Gửi dữ liệu anomaly lên cloud

### Mục tiêu
Cloud phải có đủ dữ liệu để quan sát và làm nguồn retrain.

### Telemetry hiện có nên giữ
- `temperature`
- `humidity`
- `anomaly_score`
- `is_anomaly`

### Telemetry nên bổ sung
- `model_version`
- `device_role`
- `threshold`
- optional:
  - `sample_id`
  - `raw_temperature`
  - `raw_humidity`

### Khi anomaly xảy ra
Nên có một trong hai cách:

#### Cách A — chỉ dựa vào telemetry
- gửi `is_anomaly = true`
- dùng cloud lọc dữ liệu bất thường

#### Cách B — gửi thêm anomaly event riêng
- mỗi anomaly gửi thêm 1 payload giàu thông tin
- dễ export hơn cho retrain

### Việc cần làm
1. Chỉnh code sensor để thêm `model_version`.
2. Đảm bảo telemetry anomaly lên CoreIOT đều.
3. Tạo dashboard xem anomaly score.
4. Optional: tạo Rule Chain lưu hoặc route anomaly event riêng.

### Deliverable phase này
- Dashboard thấy anomaly.
- Có lịch sử anomaly để export.
- Biết model version nào đã tạo ra dữ liệu nào.

---

## Phase 2.2 — Export dữ liệu và retrain trên PC

### Mục tiêu
Biến dữ liệu cloud thành model mới tốt hơn.

### Input dữ liệu
- telemetry từ CoreIOT
- anomaly data
- optional: dữ liệu gán nhãn tay

### Pipeline PC khuyên dùng
1. Export dữ liệu từ cloud.
2. Convert sang CSV hoặc định dạng dễ train.
3. Làm sạch dữ liệu:
   - bỏ lỗi
   - xử lý thiếu dữ liệu
   - đồng bộ timestamp
4. Gán nhãn:
   - normal
   - anomaly
   - hoặc nhiều mức nếu muốn
5. Chia train / validation / test.
6. Train model.
7. Evaluate model.
8. Xuất model mới.

### Script nên có trên PC
- `export_data.py`
- `prepare_dataset.py`
- `train_model.py`
- `evaluate_model.py`
- `export_model.py`

### Output cần có
- model mới
- metrics đánh giá
- version model mới
- changelog ngắn

### Deliverable phase này
- Có thể train lại model từ dữ liệu thực tế.
- Có file model mới sẵn sàng đưa về ESP32.

---

## Phase 2.3 — Đưa model mới về ESP32 bằng cách dễ nhất

### Recommendation
Phase đầu **không nên làm model OTA độc lập ngay**.
Nên đi đường dễ và chắc:

1. build model mới vào firmware
2. dùng **Task 1 Web OTA** để update firmware mới lên ESP32

### Vì sao nên làm vậy?
- ít thay đổi kiến trúc
- ít rủi ro hơn
- nhanh có kết quả demo
- tận dụng được kết quả của Task 1

### Deliverable phase này
- model mới được deploy lên board
- board infer bằng model mới
- có `model_version` mới trong telemetry

---

## Phase 2.4 — OTA model độc lập

### Mục tiêu
Không cần rebuild full firmware mỗi khi đổi model.

### Thiết kế khuyên dùng
- Firmware đóng vai trò runtime + model loader.
- Model là artifact riêng.
- Model lưu ở LittleFS hoặc partition riêng.
- ESP32 tải model mới từ server/storage.
- Verify checksum xong mới activate.

### Luồng đề xuất
1. PC train model mới.
2. Convert model thành artifact nhỏ gọn.
3. Upload artifact lên server.
4. ESP32 check `model_manifest.json`.
5. Nếu có `model_version` mới:
   - tải model
   - verify checksum
   - ghi storage
   - activate model mới
6. Board báo lại cloud model version mới.

### Đây là phần nâng cao
Không bắt buộc làm ngay. Nên làm sau khi Phase 2.1–2.3 đã ổn.

---

## Thứ tự triển khai Task 2 khuyên dùng

### Step 1
Bổ sung telemetry cho anomaly + `model_version`.

### Step 2
Làm dashboard / export dữ liệu.

### Step 3
Làm pipeline retrain trên PC.

### Step 4
Train model mới đầu tiên.

### Step 5
Build model mới vào firmware.

### Step 6
Dùng Web OTA của Task 1 để update model mới qua firmware.

### Step 7
Khi mọi thứ ổn, mới làm model OTA độc lập.

---

## Rủi ro của Task 2
- Dữ liệu anomaly ít hoặc nhãn chưa tốt.
- Model mới nặng quá so với RAM/flash ESP32.
- Retrain xong nhưng model mới tệ hơn model cũ.
- Không có version rõ ràng nên khó đối chiếu.
- Làm model OTA độc lập quá sớm sẽ tăng độ phức tạp mạnh.

---

## Tiêu chí hoàn thành Task 2
Task 2 được coi là xong mức cơ bản khi:
- anomaly data lên cloud đầy đủ
- có pipeline retrain trên PC
- tạo được model mới
- model mới chạy lại trên ESP32
- telemetry có `model_version` để kiểm chứng

Task 2 được coi là xong mức nâng cao khi:
- model mới được cập nhật xuống ESP32 không cần build full firmware

---

## Nhánh git đề xuất cho Task 2
- `feature/auto-retrain-tinyml`

---

# Thứ tự làm giữa 2 task

## Làm trước
### Task 1 — Web OTA
Vì task này tạo hạ tầng cập nhật từ xa, sẽ giúp task sau triển khai model mới đỡ cực hơn.

## Làm sau
### Task 2 — Auto-Retrain TinyML
Sau khi OTA firmware qua web đã ổn, việc đưa model mới về board sẽ thực tế và dễ demo hơn.

---

# Gợi ý mở chat mới để làm đúng chuẩn
Nếu mở chat mới, bạn có thể đưa thẳng một trong hai câu sau.

## Prompt cho Task 1
"Đọc file PLAN_OTA_TINYML.md trong repo này và giúp tôi làm Task 1 - OTA Update qua Web theo đúng checklist, ưu tiên sensor board trước, giữ scope đơn giản và an toàn."

## Prompt cho Task 2
"Đọc file PLAN_OTA_TINYML.md trong repo này và giúp tôi làm Task 2 - Auto-Retrain TinyML theo đúng phase, bắt đầu từ Phase 2.1 gửi anomaly lên cloud và thêm model_version."

---

# Kết luận cuối
Hiện tại cách đi tốt nhất là:

1. Hoàn tất Rule Chain hiện tại
2. Làm **Task 1 — Web OTA** trước
3. Sau đó làm **Task 2 — Auto-Retrain TinyML** theo từng phase

Nếu bám theo đúng file này, bạn có thể mở chat mới và làm tiếp từng task mà không bị lẫn scope.