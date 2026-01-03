# BẢN THUYẾT MINH THUYẾT TRÌNH

##  THIẾT KẾ VÀ TRIỂN KHAI HỆ THỐNG CẢNH BÁO CHÁY THÔNG MINH DỰA TRÊN ESP32
**Nhóm 06 - Học viện Công nghệ Bưu chính Viễn thông**

---

##  SLIDE 1: TRANG BÌA

*[Đứng thẳng, nhìn thẳng vào Hội đồng, giọng rõ ràng, tự tin]*

> "Kính thưa cô Phan Thị Lan và các bạn sinh viên,
>
> Em xin tự giới thiệu, em là [TÊN], đại diện cho Nhóm 06 trình bày đề tài: **"Thiết kế và Triển khai Hệ thống Cảnh báo Cháy Thông minh dựa trên ESP32 - Giám sát và Điều khiển Từ xa"**.
>
> Đề tài này tập trung vào việc tối ưu hóa độ trễ và độ chính xác bằng kỹ thuật Sensor Fusion trên nền tảng vi điều khiển ESP32.
>
> Xin phép cô và các bạn cho em được bắt đầu phần trình bày ạ."

---

##  SLIDE 2: NỘI DUNG TRÌNH BÀY

> "Bài thuyết trình của nhóm em sẽ được chia thành **bốn phần chính**:
>
> - **Phần I - Mở đầu**: Trình bày thực trạng, bài toán và mục tiêu kỹ thuật.
> - **Phần II - Thiết kế Hệ thống**: Kiến trúc phần cứng, phần mềm và bảo mật.
> - **Phần III - Thực nghiệm**: Các kết quả đo đạc và đánh giá hiệu suất.
> - **Phần IV - Kết luận**: Tổng kết thành tựu, hạn chế và hướng phát triển.
>
> Xin phép cô và các bạn cho em bắt đầu với Phần I ạ."

---

#  PHẦN I: MỞ ĐẦU

##  SLIDE 3: Thực trạng và Bài toán

> "Kính thưa cô và các bạn,
>
> Trước hết, cho phép em trình bày về **thực trạng nghiêm trọng** của vấn đề cháy nổ hiện nay.
>
> Theo báo cáo của CTIF năm 2022, mỗi năm trên thế giới xảy ra **hàng triệu vụ hỏa hoạn**. Riêng tại Việt Nam trong năm 2024, theo thống kê của Bộ Công an, toàn quốc đã xảy ra **4.112 vụ cháy**, làm **chết 100 người**, bị thương 89 người và gây thiệt hại tài sản ước tính khoảng **657,45 tỷ đồng**.
>
> Điều đáng lưu ý là phần lớn thương vong xảy ra do **phát hiện chậm trễ** - nghĩa là hệ thống cảnh báo không kịp thời hoặc không đủ tin cậy.
>
> *[Chỉ tay vào phần nhược điểm]*
>
> Khi phân tích các hệ thống báo cháy truyền thống, nhóm em nhận thấy **ba nhược điểm chính**:
>
> **Thứ nhất**, các hệ thống này hoạt động **biệt lập**, không kết nối mạng, do đó không thể giám sát từ xa khi chủ nhà vắng mặt.
>
> **Thứ hai**, tỷ lệ **báo động giả rất cao**, có thể lên đến hơn 20%, do nhạy cảm với nhiễu như bụi, hơi nước, hay khói nấu ăn.
>
> **Và thứ ba**, độ trễ phản hồi lớn vì phụ thuộc vào server cloud, không đạt được tính năng real-time thực sự.
>
> Từ những phân tích trên, nhóm em đặt ra bài toán: **Làm sao xây dựng một hệ thống cảnh báo cháy có độ trễ thấp, tỷ lệ báo giả thấp, và có khả năng giám sát từ xa với chi phí hợp lý?**"

---

##  SLIDE 4: Mục Tiêu Kỹ Thuật

> "Để giải quyết ba nhược điểm vừa nêu, nhóm em đã xác định **ba mục tiêu kỹ thuật cốt lõi**:
>
> *[Chỉ vào Block 1]*
>
> **Mục tiêu thứ nhất - Tốc độ phản hồi**: Nhóm em đặt mục tiêu độ trễ phản hồi tại chỗ phải **dưới 10 giây**, tuân thủ tiêu chuẩn NFPA 72 của Hoa Kỳ. Giải pháp của nhóm là sử dụng hệ điều hành thời gian thực **FreeRTOS** chạy đa luồng trên ESP32 Dual-core 240MHz.
>
> *[Chỉ vào Block 2]*
>
> **Mục tiêu thứ hai - Độ tin cậy**: Giảm tỷ lệ báo giả xuống **dưới 15%**. Để đạt được điều này, nhóm em áp dụng kỹ thuật **Sensor Fusion**, kết hợp dữ liệu từ ba cảm biến IR, MQ-2 và DHT22 với logic đa điều kiện.
>
> *[Chỉ vào Block 3]*
>
> **Mục tiêu thứ ba - Tính ổn định**: Hệ thống phải tự động kết nối lại WiFi với tỷ lệ thành công **100%** và thời gian khởi động nguội **dưới 2 giây**. Nhóm em sử dụng thư viện WiFiManager kết hợp lưu trữ cấu hình bền vững trên SPIFFS.
>
> Tiếp theo, xin phép cô và các bạn cho em chuyển sang Phần II - Thiết kế Hệ thống ạ."

---

#  PHẦN II: THIẾT KẾ HỆ THỐNG

##  SLIDE 5: Kiến Trúc Hệ Thống 3 Lớp

> "Kính thưa cô và các bạn,
>
> Hệ thống của nhóm em được thiết kế theo **kiến trúc phân lớp**, bao gồm ba lớp chính:
>
> *[Chỉ vào sơ đồ kiến trúc]*
>
> **Lớp 1 - Thu thập Dữ liệu**: Bao gồm ba loại cảm biến - cảm biến hồng ngoại phát hiện lửa, cảm biến khí MQ-2 phát hiện khói và gas, và cảm biến DHT22 đo nhiệt độ độ ẩm. Tần suất lấy mẫu là **100 mili-giây** cho IR và MQ-2, riêng DHT22 là **2 giây** do giới hạn phần cứng.
>
> **Lớp 2 - Xử lý Edge Computing**: Đây là trái tim của hệ thống. ESP32 Dual-core được phân chia nhiệm vụ - **Core 1** xử lý cảm biến và logic điều khiển, còn **Core 0** chuyên phục vụ kết nối WiFi và gửi email. Nhóm em sử dụng FreeRTOS với 3 task song song: SensorRead, LogicControl và AlertSend.
>
> **Lớp 3 - Giao tiếp Đa kênh**: Hệ thống hỗ trợ cả **cảnh báo tại chỗ** qua relay điều khiển còi, máy bơm, quạt hút, và **cảnh báo từ xa** qua giao diện web HTTP và email SMTP bảo mật SSL."

---

##  SLIDE 6: Vi Điều Khiển ESP32

> "Về lựa chọn vi điều khiển, nhóm em đã cân nhắc giữa Arduino, STM32 và ESP32. Cuối cùng, **ESP32-WROOM-32** được chọn vì những ưu điểm vượt trội sau:
>
> *[Chỉ vào bảng thông số]*
>
> **Về hiệu năng**: CPU dual-core Xtensa LX6 chạy ở tần số **240MHz**, đạt hiệu năng 600 DMIPS, đủ mạnh để chạy đa luồng FreeRTOS.
>
> **Về bộ nhớ**: 520KB SRAM và 4MB Flash, đủ để lưu trữ firmware, giao diện web và cấu hình hệ thống trên SPIFFS.
>
> **Điểm then chốt nhất**: ESP32 tích hợp sẵn **WiFi và Bluetooth**, bộ **ADC 12-bit** với độ phân giải 0.805 mV/step - rất phù hợp để đọc tín hiệu analog từ cảm biến MQ-2.
>
> Ngoài ra, ESP32 hỗ trợ **FreeRTOS native**, giúp việc lập trình đa luồng trở nên đơn giản và hiệu quả hơn."

---

##  SLIDE 7: Cảm Biến Khí MQ-2

> "Tiếp theo, cho phép em trình bày về cảm biến khí **MQ-2** - thành phần quan trọng nhất trong việc phát hiện khói và rò rỉ gas.
>
> *[Chỉ vào hình ảnh và thông số]*
>
> MQ-2 sử dụng vật liệu bán dẫn oxit thiếc SnO2, có khả năng phát hiện khí LPG, khói và Hydro trong dải đo từ 300 đến 10.000 ppm.
>
> *[Chỉ vào công thức]*
>
> Về thuật toán chuyển đổi tín hiệu, nhóm em thực hiện **hai bước**:
>
> **Bước 1**: Tính điện trở cảm biến Rs từ điện áp đầu ra theo công thức trong hộp màu xanh. Với điện áp cấp 5V và điện trở tải 10 kilo-ohm.
>
> **Bước 2**: Áp dụng phương trình đặc tuyến log-log để tính nồng độ PPM, với hai hằng số **a = 672** và **b = -2.06** được lấy từ datasheet cho đường cong LPG và khói. Giá trị R0 là điện trở trong không khí sạch, đã được nhóm em hiệu chuẩn là **18.7 kilo-ohm**."

---

##  SLIDE 8: Cảm Biến IR và DHT22

> "Bên cạnh MQ-2, hệ thống còn sử dụng hai cảm biến hỗ trợ:
>
> **Cảm biến lửa hồng ngoại IR**: Hoạt động trong dải quang phổ 760 đến 1100 nano-mét, với góc quét 60 độ và thời gian đáp ứng cực nhanh - **dưới 15 micro-giây**. Tuy nhiên, cảm biến này có nhược điểm là dễ bị nhiễu bởi ánh sáng mặt trời trực tiếp hoặc đèn Halogen công suất cao.
>
> *[Chuyển sang DHT22]*
>
> **Cảm biến DHT22**: Đo nhiệt độ trong dải -40 đến 80 độ C với sai số chỉ ±0.5 độ. Điều quan trọng là DHT22 đóng vai trò **người kiểm chứng** trong logic Fusion:
>
> - Nếu cháy thật, khói tăng đi kèm với nhiệt độ **tăng nhanh**.
> - Nếu chỉ là hơi nước từ nấu ăn, khói tăng nhưng nhiệt độ **không đổi hoặc giảm**.
>
> Đây chính là cơ sở để thuật toán Sensor Fusion loại bỏ báo động giả."

---

##  SLIDE 9: Thuật Toán Sensor Fusion

> "Đây là phần **cốt lõi** của đề tài - thuật toán Sensor Fusion.
>
> *[Chỉ vào bảng kịch bản]*
>
> Xin cô và các bạn quan sát bảng so sánh này. Khi chỉ dùng đơn cảm biến, nhiều kịch bản an toàn bị **báo nhầm**:
>
> - Nấu ăn: MQ-2 báo động do hơi nước, nhưng nhiệt độ chỉ 30 độ → An toàn.
> - Nắng chiếu qua cửa: IR báo động, nhưng nhiệt độ chỉ 28 độ → An toàn.
> - Hút thuốc: MQ-2 báo động, nhưng nhiệt độ chỉ 25 độ → An toàn.
>
> Chỉ khi **cháy thật**, cả ba cảm biến đều vượt ngưỡng cùng lúc.
>
> *[Chỉ vào công thức]*
>
> Công thức quyết định của nhóm em là: **Alarm được kích hoạt khi** - IR phát hiện lửa VÀ nhiệt độ trên 45 độ, HOẶC gas cao VÀ nhiệt độ trên 40 độ, HOẶC nhiệt độ vượt ngưỡng nguy hiểm, HOẶC cường độ IR vượt ngưỡng.
>
> Logic AND trong điều kiện đầu tiên chính là **chìa khóa** giúp giảm 50% báo động giả so với hệ thống đơn cảm biến."

---

##  SLIDE 10: Kiến Trúc FreeRTOS

> "Về kiến trúc phần mềm, nhóm em sử dụng **FreeRTOS** để quản lý đa tác vụ song song.
>
> *[Chỉ vào bảng phân bố task]*
>
> Hệ thống có 4 task chính:
>
> - **SensorReadTask**: Chạy trên Core 1 với ưu tiên cao nhất, chu kỳ 100ms, đọc dữ liệu từ ADC và DHT.
> - **LogicControlTask**: Cũng trên Core 1, chạy thuật toán Fusion và điều khiển Relay.
> - **AlertSendTask**: Chạy trên **Core 0 riêng biệt**, chờ sự kiện từ Queue rồi gửi email SMTP.
> - **Loop Task**: Xử lý nút reset cứng và LED báo trạng thái WiFi.
>
> *[Chỉ vào phần cơ chế đồng bộ]*
>
> Để tránh xung đột dữ liệu, nhóm em sử dụng **Mutex Semaphore** bảo vệ vùng nhớ chung. Đồng thời, **Message Queue** được dùng để truyền yêu cầu cảnh báo từ Logic Task sang Alert Task một cách bất đồng bộ - nghĩa là việc gửi email không làm chậm quá trình đọc cảm biến."

---

##  SLIDE 11-14: Hệ Thống Bảo Mật

> "Một trong những điểm nhấn của đề tài là hệ thống bảo mật **4 lớp**, tuân thủ khuyến nghị OWASP:
>
> **Lớp 1 - Xác thực Token-based**: Thay vì gửi mật khẩu trong mỗi request, sau khi đăng nhập thành công, hệ thống cấp một **token ngẫu nhiên 32 ký tự**. Token này có thời hạn 5 phút và tự động gia hạn khi có hoạt động. Đặc biệt, hệ thống hỗ trợ **tối đa 10 token đồng thời**, cho phép nhiều thiết bị truy cập cùng lúc.
>
> **Lớp 2 - Chống Brute-force**: Sử dụng thuật toán **khóa tiệm tiến** - sau 5 lần đăng nhập sai, tài khoản bị khóa 30 giây. Lần thứ hai bị khóa 90 giây, lần thứ ba 270 giây, tăng theo cấp số nhân. Cơ chế này giảm nguy cơ bị tấn công từ 100% xuống **dưới 0.001%**.
>
> **Lớp 3 - Quản lý mật khẩu**: Hệ thống tự động phát hiện mật khẩu mặc định và **bắt buộc đổi** ngay lần đăng nhập đầu tiên qua một modal không thể đóng.
>
> **Lớp 4 - Bảo vệ API**: Middleware kiểm tra token trước khi cho phép truy cập các endpoint nhạy cảm như /data, /settings, /history. Request không hợp lệ sẽ nhận HTTP 401 Unauthorized."

---

##  SLIDE 15-16: Giao Diện Web và Dịch Vụ Mạng

> "Về giao diện người dùng, nhóm em phát triển ứng dụng web sử dụng **React 18, TypeScript và Vite**.
>
> *[Chỉ vào hình ảnh dashboard]*
>
> Dashboard giám sát bao gồm:
> - **Header** hiển thị logo 'Hệ Thống PCCC Thông Minh' và trạng thái kết nối.
> - **Status Banner** chuyển từ xanh sang đỏ nhấp nháy khi có cảnh báo.
> - **4 Gauge Chart** hiển thị nhiệt độ, độ ẩm, nồng độ gas và cường độ lửa.
> - **Live Chart** biểu đồ đường cập nhật theo thời gian thực.
>
> Điểm đặc biệt là cơ chế **polling động** - bình thường cập nhật mỗi **1 giây**, nhưng khi có cảnh báo sẽ tăng tần suất lên **400 mili-giây** để người dùng theo dõi sát hơn.
>
> *[Chỉ vào phần email]*
>
> Về cảnh báo từ xa, hệ thống gửi email qua **SMTP Gmail port 465 với SSL/TLS**. Nội dung email định dạng HTML bao gồm bảng dữ liệu cảm biến tại thời điểm xảy ra sự cố."

---

##  SLIDE 17-18: Thiết Kế Mạch In PCB

> "Về phần cứng, nhóm em đã thiết kế mạch in PCB chuyên dụng.
>
> *[Chỉ vào sơ đồ khối nguồn]*
>
> Mạch nhận nguồn đầu vào **12V DC**, sử dụng **Buck Converter LM2596** để hạ áp xuống 5V thay vì dùng IC 7805. Điều này giúp tăng hiệu suất năng lượng và giảm nhiệt tỏa ra.
>
> Để bảo vệ vi điều khiển, các tín hiệu điều khiển relay được **cách ly hoàn toàn** qua Optocoupler PC817, ngăn chặn dòng ngược Back-EMF từ cuộn dây relay gây treo MCU.
>
> *[Chỉ vào hình ảnh PCB thực tế]*
>
> Đây là sản phẩm PCB hoàn thiện - mạch in 2 lớp, phủ solder mask đen, tích hợp socket cho ESP32-DevKit, 3 relay 12V và terminal block để đấu nối dây điện.
>
> Tiếp theo, xin phép cô và các bạn cho em chuyển sang Phần III - Thực nghiệm Hệ thống ạ."

---

#  PHẦN III: THỰC NGHIỆM HỆ THỐNG

##  SLIDE 19: Hiệu Chuẩn Cảm Biến MQ-2

> "Kính thưa cô và các bạn,
>
> Trước khi vận hành hệ thống, nhóm em đã thực hiện quy trình **hiệu chuẩn** cảm biến MQ-2.
>
> *[Chỉ vào quy trình]*
>
> Theo datasheet, MQ-2 cần được sấy nóng liên tục **24 giờ** để vật liệu SnO2 ổn định. Sau đó, cảm biến được đặt trong môi trường không khí sạch để đo điện trở chuẩn R0.
>
> Công thức tính R0 là: **R0 = Rs / 9.83**, trong đó 9.83 là hệ số không khí sạch từ datasheet.
>
> *[Chỉ vào kết quả]*
>
> Kết quả hiệu chuẩn của nhóm em: R0 = **18.7 kilo-ohm**, rất gần với giá trị lý thuyết 20 kilo-ohm. Giá trị này được lưu vào firmware làm mốc chuẩn cho tất cả các phép tính nồng độ PPM."

---

##  SLIDE 20: Thiết Lập Ngưỡng Cảnh Báo

> "Việc thiết lập ngưỡng cảnh báo là bước quan trọng để cân bằng giữa **độ nhạy** và **khả năng chống báo giả**.
>
> *[Chỉ vào bảng ngưỡng]*
>
> **Ngưỡng khí Gas**: Nhóm em chọn **1.500 PPM**, tương đương 30% giới hạn nổ dưới LEL, tuân thủ tiêu chuẩn châu Âu EN 50194. Mức này cao gấp **58 lần** nhiễu nền bình thường, đảm bảo loại bỏ hoàn toàn báo giả từ hơi cồn hay khói nấu ăn nhẹ.
>
> **Ngưỡng nhiệt độ**: 45 độ C cho cảnh báo sớm trong thuật toán Fusion, và **50 độ C** cho cảnh báo nguy hiểm, tiệm cận tiêu chuẩn UL 521.
>
> **Ngưỡng lửa IR**: Giá trị ADC **3.000** trên thang 4.095, chỉ kích hoạt khi có ngọn lửa thực sự ở khoảng cách dưới 1.5 mét."

---

##  SLIDE 21: Phân Tích Độ Ổn Định Tín Hiệu

> "Để kiểm chứng độ tin cậy của dữ liệu đầu vào, nhóm em đã thu thập **1.000 mẫu** trong 100 giây ở trạng thái tĩnh.
>
> *[Chỉ vào biểu đồ]*
>
> Kết quả cho thấy:
>
> - **Nhiệt độ DHT22**: Ổn định trong khoảng 27.70 - 27.80 độ C, không có gai nhiễu, chứng tỏ nguồn cấp ổn định.
>
> - **Khí Gas MQ-2**: Nhiễu nền dao động từ 24-30 PPM, biên độ chỉ **6 PPM** - rất nhỏ so với ngưỡng 1.500 PPM.
>
> - **Cảm biến Lửa IR**: Bão hòa ở mức 4.095, không bị nhiễu bởi đèn huỳnh quang trong phòng.
>
> Dữ liệu này chứng minh phần cứng có độ ổn định cao, đủ điều kiện làm đầu vào cho thuật toán Sensor Fusion."

---

##  SLIDE 22: Kết Quả Độ Trễ

> "Đây là kết quả quan trọng nhất - **độ trễ phản hồi** của hệ thống.
>
> *[Chỉ vào bảng thống kê]*
>
> Nhóm em đã đo đạc trên 175 mẫu đọc cảm biến và 125 mẫu xử lý logic:
>
> - Thời gian đọc cảm biến trung bình: **464.67 micro-giây**, tức chưa đến nửa mili-giây.
> - Thời gian xử lý logic Fusion: chỉ **2.87 micro-giây**, gần như tức thời.
>
> *[Chỉ vào so sánh NFPA 72]*
>
> Tổng độ trễ cảnh báo tại chỗ là **dưới 0.5 mili-giây**. So với tiêu chuẩn NFPA 72 yêu cầu dưới 10 giây, hệ thống của nhóm em **vượt trội hơn 20.000 lần**, hoàn toàn đáp ứng yêu cầu Real-time.
>
> Riêng email cảnh báo có độ trễ trung bình **6.4 giây**, phụ thuộc vào mạng, nhưng chạy bất đồng bộ nên không ảnh hưởng đến phản hồi tại chỗ."

---

##  SLIDE 23-24: Đánh Giá Độ Chính Xác

> "Để đánh giá độ chính xác, nhóm em đã thiết kế **80 kịch bản thử nghiệm**: 40 trường hợp có cháy thực và 40 trường hợp an toàn.
>
> *[Chỉ vào ma trận nhầm lẫn]*
>
> Kết quả:
> - **35 True Positive**: Báo đúng có cháy.
> - **35 True Negative**: Báo đúng an toàn.
> - **5 False Positive**: Báo nhầm - do đèn Halogen hoặc hơi nước liên tục.
> - **5 False Negative**: Bỏ sót - do lửa nhỏ hoặc khoảng cách xa.
>
> Từ đó, các chỉ số hiệu suất đạt được:
> - **Accuracy: 87.5%**
> - **Sensitivity: 87.5%**
> - **False Positive Rate: chỉ 12.5%**
>
> *[Chỉ vào biểu đồ so sánh]*
>
> So với việc chỉ dùng đơn cảm biến - MQ-2 alone có FPR 25%, IR alone có FPR 30% - thuật toán Sensor Fusion của nhóm em đã **cải thiện Accuracy thêm 10%** và **giảm 50% báo động giả**."

---

##  SLIDE 25-26: Độ Ổn Định Kết Nối

> "Về độ ổn định kết nối, nhóm em thực hiện kiểm thử **Stress Test** - ngắt nguồn router đột ngột 10 lần liên tiếp.
>
> *[Chỉ vào kết quả]*
>
> Kết quả:
> - Tỷ lệ tự động kết nối lại: **100%** - không lần nào bị treo.
> - Thời gian khôi phục trung bình MTTR: **8.27 giây**.
>
> Về thời gian khởi động nguội Cold Boot:
> - Init phần cứng: chỉ **3 mili-giây**.
> - Load cấu hình từ SPIFFS: **261 mili-giây**.
> - Kết nối WiFi: trung bình **938 mili-giây**.
>
> **Tổng cộng chỉ 1.2 giây** - nhanh hơn 3 lần so với các hệ thống thương mại thông thường."

---

##  SLIDE 27: Công Suất Tiêu Thụ

> "Về năng lượng, nhóm em đo tại đầu vào 12V:
>
> - Chế độ chờ: **180 mA**, tương đương **2.16 Watt**.
> - Chế độ kích hoạt đầy đủ: **300 mA**, tương đương **3.6 Watt**.
>
> Với pin dự phòng 10.000 mAh và hiệu suất chuyển đổi 80%, hệ thống có thể hoạt động **13.7 giờ** khi mất điện lưới. Để đạt chuẩn NFPA 72 yêu cầu 24 giờ, nhóm em khuyến nghị nâng cấp lên pin 20.000 mAh.
>
> Xin phép cô và các bạn cho em chuyển sang phần cuối cùng - Kết luận ạ."

---

#  PHẦN IV: KẾT LUẬN

##  SLIDE 28: Tổng Kết Kết Quả

> "Kính thưa cô và các bạn,
>
> Tổng kết lại, đề tài của nhóm em đã đạt được **năm đóng góp chính**:
>
> **Thứ nhất**, xây dựng thành công kiến trúc đa luồng FreeRTOS trên ESP32 Dual-core, đạt thời gian phản hồi **dưới 0.5 mili-giây**, vượt chuẩn NFPA 72 hơn 6.600 lần.
>
> **Thứ hai**, thuật toán Sensor Fusion kết hợp IR, MQ-2, DHT22 giúp **giảm 50% báo động giả** và đạt Accuracy 87.5%.
>
> **Thứ ba**, hệ thống bảo mật 4 lớp với xác thực Token-based và chống Brute-force, tuân thủ khuyến nghị OWASP.
>
> **Thứ tư**, khả năng tự phục hồi kết nối WiFi **100%** với thời gian cold boot chỉ 1.2 giây.
>
> **Và thứ năm**, chi phí triển khai chỉ khoảng **25 đô la** - bằng 1/3 giá hệ thống thương mại, nhưng vẫn đạt 93% hiệu suất."

---

##  SLIDE 29: Hạn Chế Tồn Tại

> "Tuy nhiên, nhóm em cũng nhận thức được những **hạn chế** cần cải thiện:
>
> **Về độ chính xác**: 87.5% vẫn thấp hơn ngưỡng thương mại 92%. Các trường hợp False Negative xảy ra khi lửa nhỏ hoặc khoảng cách xa, do giới hạn của cảm biến IR giá rẻ.
>
> **Về hạ tầng mạng**: Cảnh báo email/web bị vô hiệu khi mất Internet. Hệ thống hiện thiếu kênh dự phòng như GSM hoặc LoRa.
>
> **Về bảo mật**: Vẫn sử dụng HTTP thuần, chưa có HTTPS. Mật khẩu lưu plaintext trong SPIFFS.
>
> **Về nguồn dự phòng**: Pin 10.000 mAh chỉ đủ 13.7 giờ, chưa đạt chuẩn NFPA 72 yêu cầu 24 giờ."

---

##  SLIDE 30-32: Hướng Cải Tiến

> "Dựa trên những hạn chế đó, nhóm em đề xuất các **hướng cải tiến**:
>
> **Về phần cứng**: Thay thế MQ-2 bằng cảm biến Figaro TGS5042 chuyên dụng, nâng Accuracy lên trên 92%. Bổ sung module GSM SIM800L hoặc LoRa SX1278 làm kênh truyền thông dự phòng.
>
> **Về thuật toán**: Áp dụng Machine Learning với Random Forest để tự chỉnh ngưỡng theo môi trường. Triển khai TinyML trực tiếp trên ESP32 để nhận diện pattern phức tạp.
>
> **Về bảo mật**: Tích hợp HTTPS qua mbedTLS, mã hóa mật khẩu bằng bcrypt, và hỗ trợ OTA firmware update từ xa.
>
> **Về mở rộng**: Xây dựng mạng lưới đa điểm qua MQTT Broker, tích hợp Cloud để lưu trữ dữ liệu lâu dài và phân tích Big Data."

---

##  SLIDE 33: Đánh Giá Khả Năng Triển Khai

> "Về khả năng triển khai thực tế:
>
> **Phần cứng** đã hoạt động ổn định - PCB 2 lớp không có hiện tượng nhiễu EMI, relay cách ly an toàn qua Optocoupler.
>
> **Phần mềm** FreeRTOS chạy ổn định không crash, giao diện web responsive với trải nghiệm người dùng tốt.
>
> Nhóm em khuyến nghị bổ sung **Fuse bảo vệ quá dòng**, **TVS diode chống sét**, và **vỏ nhựa chống cháy UL94 V-0** cho sản phẩm hoàn thiện."

---

##  SLIDE 34: Định Hướng Ứng Dụng

> "Cuối cùng, nhóm em định hướng ứng dụng trong các lĩnh vực:
>
> - **Hộ gia đình và chung cư**: Tích hợp Google Home/Alexa, chi phí triển khai 30-40 đô la/căn hộ.
>
> - **Nhà xưởng và kho bãi**: Mạng lưới Multi-node qua MQTT, kết nối PLC/SCADA hiện có, chi phí 150-300 đô la/500m².
>
> - **Trường học và bệnh viện**: Ưu tiên độ tin cậy cao, tích hợp hệ thống PA, tuân thủ QCVN 06:2021.
>
> - **Vùng nông thôn**: Dùng LoRa/GSM thay WiFi, UPS tích hợp bắt buộc, giao diện đơn giản plug-and-play."

---

##  SLIDE CUỐI: Lời Cảm Ơn

> "Kính thưa cô và các bạn,
>
> Đó là toàn bộ phần trình bày của nhóm em về đề tài **Thiết kế và Triển khai Hệ thống Cảnh báo Cháy Thông minh dựa trên ESP32**.
>
> Nhóm em xin chân thành cảm ơn **cô Phan Thị Lan** đã tận tình hướng dẫn trong suốt quá trình thực hiện đề tài. Cảm ơn các bạn đã chú ý lắng nghe.
>
> Nhóm em xin sẵn sàng nhận mọi câu hỏi và góp ý từ cô và các bạn để hoàn thiện đề tài tốt hơn ạ.
>
> **Em xin trân trọng cảm ơn!**"

---



