#include <Servo.h> //Import library function untuk control servo (servo.attach and servo.write)
                   //Karena servo menggunakan PWM (Pulse Width Modulation) tidak hanya HIGH/LOW
                   //Pulse Width tersebut menentukan derajat servo, library ini membantu agar kita 
                   //cukup mengatur derajatnya saja dan semua langsung dikerjakan

Servo scanServo;  //Membuat object ldengan nama scanServo yang memiliki tipe Servo, jadi Servo adalah class dari library
                  //sedangkan scanServo adalah object/instance dari class, function ini digunakan untuk attach, write, read, detach

// Motor Driver Initialization
// ===========================
int IN1 = 8;
int IN2 = 9;
int IN3 = 10;
int IN4 = 11; //Initialize 4 variable integer yang digunakan untuk store pin Arduino UNO ke pin input pada L298N
              //This basic aja, karna kita nanti tinggal reuse IN1-IN4 di statement yang kita perlu pakai pin untuk pergerakan
              //motor berdasarkan data sensor


// Ultrasonic Sensor Initialization
// ================================
int trigPin = 6;
int echoPin = 7;  //Initialize 2 variable integer yang digunakan untuk store pin UNO ke 2 pin pada HC-SR04
                  //Behavior 2 pin ini beda, Trig sebagai penanda output dari arduino ke sensor untuk menembak gelombang
                  //sedangkan Echo sebagai penanda input dari HC-SR04 ke arduino untuk menerima data pantulan gelombang

long duration;
int distance;   //Initialize 2 variable lagi untuk store data durasi pancaran gelombang dan perhitungannya ke bentuk centimeter
                //duration menggunakan long karena datanya besar bisa mencapai puluhan/ratusan ribu microseconds
                //distance menggunakan int karena menyimpan hasil perhitungan duration dengan output bilangan bulat dalam bentuk cm


// Servo Initialization
// ====================
int servoPos = 90;
int scanDirection = 1;  //servoPos digunakan untuk menyimpan data awal posisi sudut servo, kenapa 90? karna itu posisi tengah
                        //karna pada bagian setup, inisialisasi robot selalu memaksa agar dimanapun posisi servo, dia harus bergerak ke 90 derajat dahulu
                        //scanDirection digunakan untuk arah pergerakan servo (1 berarti kanan -1 berarti kiri)


// Robot status
// ============
bool targetLocked = false;
bool backingOff = false;    //boolean menyimpan 2 state (T/F), dan dua variable ini digunakan untuk 
                            //inisialisasi dulu bahwa kedua statusnya masih false
unsigned long lastScan = 0; //menyimpan data millis yang terus berakumulasi, jadi menggunakan unsigned long agar tidak overflow


// Setting Up the code
// ===================
void setup(){ //usual setting up + serial monitor
  Serial.begin(9600);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT); //Motor driver output

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT); //HC-SR04 Output and Input for pulse

  scanServo.attach(5);
  scanServo.write(90); //Servo initialization using pin 5 dan selalu menggunakan 90 sebagai posisi awal servo

  Serial.println("Robot Ready"); //Output serial monitor saat robot menyala
}


// Ultrasonic Filtered (8 Sample)
// ==============================
int readDistance(){ //Read distance dari HC-SR04 return nilai dengan int dalam centimeter
  const int samples = 8;  //Sensor dibaca sebanyak 8 kali untuk menghindari noise (const berarti 8 jumlah sample tetap)
  int values[samples];  //Declare array sebanyak 8 element, store 8 reading sensor dahulu

  for(int i = 0; i < samples; i++){ //Limit loop adalah 8 kali masing-masing mengulangi pengukuran HC-SR04 1x

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);  //Standard way to use HC-SR04, memberikan trigger HIGH selama 10 micro untuk memancarkan gelombang

    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH, 30000); //belum berbentuk jarak, melainkan durasi pin Echo pada kondisi HIGH (micro)

    values[i] = duration * 0.034 / 2; //konversi jarak = waktu x kecepatan suara bagi 2 (pergi dan kembali)

    delay(5);
  }

  int minVal = values[0]; 
  int maxVal = values[0]; //data pertama adalah nilai terkecil dan terbesar (asumsi)
  int sum = 0;

  for(int i = 0; i < samples; i++){

    if(values[i] < minVal) minVal = values[i]; //mencari nilai terkecil
    if(values[i] > maxVal) maxVal = values[i]; //mencari nilai terbesar

    sum += values[i]; //menjumlahkan seluruh data yang ada
  }

  sum -= minVal;
  sum -= maxVal; //nilai terkecil dan nilai terbesar akan dibuang (trimmed mean)

  return sum / (samples - 2); //rata ratanya dihitung dengan membuang 2 data (terkececil dan terbesar)
}


// Motor Control
// =============
void maju(){ //Menggerakan motor untuk maju, jadi saat function dipanggil motor akan maju
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void mundur(){ //Menggerakan motor untuk mundur, jadi saat function dipanggil motor akan mundur
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void kiri(){ //Menggerakan motor untuk ke kiri, jadi saat function dipanggil motor akan berbelok ke kiri
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void kanan(){ //Menggerakan motor untuk ke kanan, jadi saat function dipanggil motor akan berbelok ke kanan
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotor(){ //Saat function dipanggil, motor akan berhenti
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}


// Servo Searching
// ===============
void scanServoSearch(){ //Menggerakan servo ke kiri dan kanan untuk mencari objek (hanya di call saat object found)

  unsigned long now = millis(); //Output waktu sejak Arduino pertama kali menyala dalam satuan mili (store to var "now")

  if(now - lastScan > 200){ //servo hanya bergerak jika sudah lebih dari 200mili sejak pergerakan terakhir (non blocking)

    servoPos += scanDirection * 4; //Menggerakan servo by 4 derajat

    if(servoPos >= 160){ //Mencapai 160 derajat atau melewati, posisi dikunci dan arah scan akan berbalik ke kiri

      servoPos = 160;
      scanDirection = -1;
    }

    if(servoPos <= 20){ //Mencapai 20 derajat atau melewati, posisi dikunci dan arah scan akan berbalik ke kanan

      servoPos = 20;
      scanDirection = 1;
    }

    scanServo.write(servoPos); //Setelah posisi baru terhitung, nilai akan dikirim ke servo

    lastScan = now; //Store waktu sekarang setelah servo bergerak, akan dieksekusi kembali setelah 200ms
  }
}


// Main Loop of The Code
// =====================
void loop(){

  distance = readDistance();  //Call function readDistance yang sudah di filter trimmed mean

  if(distance > 40){  //Threshold apabila distance diatas 40cm maka distance otomatis 0
    distance = 0;
  }

  Serial.print("Distance: "); //Resultin in approx 800 byte/s, suitable to use 9600
  Serial.print(distance);
  Serial.print(" | Servo: ");
  Serial.print(servoPos);
  Serial.print(" | Backoff: ");
  Serial.println(backingOff); //Output Serial monitor aja untuk status distance, degree servo, dan apakah robot mundur


  // Mengaktifkan Mode Mundur
  // ========================
  if(distance > 0 && distance <= 5){  //Cek apakah objek berada pada jarak 0 sampai 5cm, if yes maka masuk mode mundur
    backingOff = true;
  }


  // Mode Mundur
  // ===========
  if(backingOff){ //Kalau robot sedang berada papda mode mundur, tidak mengecek jarak lagi
    targetLocked = true; //Memiliki target sebelumnya meskipun robot sedang mundur

    mundur(); //Execute function set untuk robot berjalan mundur

    if(distance == 0 || distance > 20){ //Syarat agar robot bisa keluar dari mode mundur, 0 (hilang) atau jarak 20cm
      backingOff = false; //Keluar dari state mundur
      targetLocked = false; //Tidak lagi mengunci target
    }
  }


  // Follow the Target
  // =================
  else if(distance > 5){  //If jarak lebih dari 5cm, maka

    targetLocked = true;  //Mengunci target sesuai arah pandang servo

    if(servoPos < 60){  //If target terdeteksi saat posisi servo < 60, maka belok kiri
      kiri();
    }

    else if(servoPos > 120){  //If target terdeteksi saat posisi servo > 120, maka belok kanan
      kanan();
    }

    else{ //Else pada range di sekitar tengah, maka maju
      maju();
    }
  }


  // Target Menghilang
  // =================
  else{ //tidak ada target valid
    targetLocked = false; //tidak memiliki target

    stopMotor();  //maka motor terhenti sambil menunggu target baru
  }


  // Search Mode Again
  // =================
  if(!targetLocked){  //berarti saat kondisi tidak sedang mengunci target, maka
    scanServoSearch();  //memanggil function ini untuk menggerakan pandangan lagi
  }

  delay(60); //delay untuk 1 cycle loop
}