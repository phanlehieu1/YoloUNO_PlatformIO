# Báo Cáo Triển Khai Tính Năng OTA Update Qua Web

Tài liệu này giải thích chi tiết về cơ chế hoạt động của tính năng Cập nhật Firmware từ xa (Over-The-Air - OTA) qua giao diện Web vừa được tích hợp vào dự án. Tài liệu được viết nhằm mục đích giúp các thành viên trong nhóm hiểu rõ luồng code và có tư liệu để viết báo cáo môn học/đồ án.

---

## 1. Tổng Quan Về OTA Web

OTA Web là phương pháp cho phép chúng ta nạp code mới (firmware) cho vi điều khiển ESP32 thông qua mạng WiFi nội bộ (LAN) thay vì phải cắm dây cáp USB trực tiếp vào máy tính. 

**Ưu điểm trong dự án này:**
- Giúp dễ dàng cập nhật code cho các board (cả Sensor và Actuator) khi chúng đã được lắp đặt ở những vị trí khó tiếp cận.
- Là tiền đề bắt buộc cho Task 2 (Auto-Retrain TinyML): Dùng kênh OTA này để vận chuyển các bản model AI mới được huấn luyện lại xuống thẳng vi điều khiển.

---

## 2. OTA Hoạt Động Như Thế Nào Trong Code Hiện Tại?

Hệ thống sử dụng thư viện **ElegantOTA** kết hợp với **AsyncWebServer** để tạo ra một trang web ẩn chuyên dùng cho việc upload firmware.

### 2.1. Cấu hình Web Server và OTA (`task_webserver.cpp`)
- Board ESP32 mở một Web Server ở cổng 80 (`AsyncWebServer server(80);`).
- Thư viện OTA được "gắn" vào Web Server này thông qua lệnh: 
  ```cpp
  ElegantOTA.begin(&webServer());
  ```
  Nhờ lệnh này, ESP32 sẽ tự động sinh ra một đường dẫn ẩn: `http://<IP_của_Board>/update`.
- Hàm `ElegantOTA.loop();` được gọi liên tục bên trong `Webserver_reconnect()` để duy trì tiến trình nạp file mà không làm treo các task khác (như đọc cảm biến hay IoT).

### 2.2. Kiểm soát phiên bản (`global.h` & `global.cpp`)
- Để biết chắc chắn việc nạp code không dây có thành công hay không, một biến cứng lưu phiên bản được tạo ra:
  ```cpp
  String getFirmwareVersion() {
    return "fw_v1"; 
  }
  ```
- Mỗi lần có code mới, người lập trình chỉ cần đổi chuỗi này thành `"fw_v2"`, `"fw_v3"`...

### 2.3. Khởi chạy hệ thống (`main.cpp`)
- Ngay khi ESP32 khởi động, phiên bản firmware sẽ được in ra Serial Monitor:
  ```cpp
  Serial.printf("[Setup] Firmware Version: %s\n", getFirmwareVersion().c_str());
  ```
- **Hỗ trợ đa thiết bị:** Ban đầu chỉ có Sensor board chạy WebServer. Hiện tại, tính năng này đã được kích hoạt cho cả **Actuator board**. Bất kể board nào khởi động, hàm `Webserver_reconnect()` cũng sẽ được gọi, giúp cả 2 board đều có khả năng nhận update OTA.

---

## 3. Hướng Dẫn Test OTA (Dùng để quay video Demo/Báo cáo)

Để chứng minh OTA hoạt động, hãy thực hiện kịch bản test theo các bước sau:

**Bước 1: Nạp bản gốc (Qua USB)**
1. Mở file `src/global.cpp`, đảm bảo hàm trả về phiên bản là `"fw_v1"`.
2. Cắm cáp USB và dùng PlatformIO nạp code vào ESP32 như bình thường.
3. Mở Serial Monitor, nhìn dòng log đầu tiên để xác nhận: `[Setup] Firmware Version: fw_v1`.
4. Tìm và ghi lại địa chỉ IP mạng nội bộ của ESP32 (in trên Serial).

**Bước 2: Tạo bản cập nhật mới (Không nạp qua USB)**
1. Mở file `src/global.cpp`, sửa `"fw_v1"` thành `"fw_v2"`.
2. Rút cáp USB hoặc đơn giản là KHÔNG nhấn nút Upload.
3. Trong PlatformIO, chọn mục **Build** (biểu tượng dấu tick `✓`) để biên dịch code.
4. Chờ PlatformIO báo SUCCESS. File firmware đã được tạo ra và thường nằm ở đường dẫn: 
   `.pio/build/<tên_board>/firmware.bin`

**Bước 3: Update qua Web OTA**
1. Mở trình duyệt web (Chrome/Edge) trên máy tính (phải dùng chung mạng WiFi với ESP32).
2. Truy cập vào địa chỉ: `http://<IP_của_Board>/update` (Ví dụ: `http://192.168.1.50/update`).
3. Giao diện ElegantOTA sẽ hiện ra. Chọn mục **Firmware**, sau đó bấm **Browse** và tìm tới file `firmware.bin` vừa được tạo ở Bước 2.
4. Nhấn **Upload** và chờ thanh tiến trình chạy đến 100%.

**Bước 4: Xác nhận kết quả**
1. Sau khi đạt 100%, trang web sẽ báo thành công và ESP32 tự động khởi động lại (Reboot).
2. Quan sát trên Serial Monitor hoặc Cloud, nếu log hiện lên dòng `[Setup] Firmware Version: fw_v2` thì tính năng OTA đã hoạt động hoàn hảo!

---

## 4. Ý Nghĩa Của Tính Năng Này Đối Với Đồ Án (Gợi ý viết báo cáo)

Khi đưa phần này vào quyển báo cáo, có thể nhấn mạnh các điểm sau:
- **Tính khả thi thực tế:** Khắc phục nhược điểm của các hệ thống IoT truyền thống (phải tháo dỡ thiết bị để nạp lại code khi có lỗi hoặc cần nâng cấp).
- **Kiến trúc linh hoạt:** Nhờ kết hợp FreeRTOS và AsyncWebServer, tiến trình tải file cập nhật (OTA) diễn ra đồng thời, không làm gián đoạn nghiêm trọng tiến trình vận hành lõi của hệ thống.
- **Mở rộng cho Machine Learning (TinyML):** Việc triển khai thành công OTA Firmware là bệ phóng cho tính năng **Auto-Retrain Model AI** ở giai đoạn sau. Thay vì phải xuống hiện trường cắm dây để nạp model AI mới, hệ thống giờ đây có thể tự động tiếp nhận model mới qua luồng OTA này.
