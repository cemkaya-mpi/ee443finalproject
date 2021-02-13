convert.py ile wav dosyalari binary dataya dönüştürülebilir.

Bu dosyaları SD Card üzerinde fat32 dosya sisteminin root directorysine
 song1.bin, song2.bin gibi isimlerle yazın.

Kaynak kodunda da bu şarkı adlarını changesong() içinde değiştirin.

Alterada proje dosyasını açıp bakarsanız compiler optionsta
  +++define
  +++-l"C:Users:.."
şeklinde seçenekler göreceksiniz. define soc V seçiyor. -l ise kütüphaneleri linkliyor.
Çalıştırılacak bilgisayara ve boarda göre ayarlanmaları önemli. C standardı ise
inline assembly anlayacak herhangi bir standart olabilir.
