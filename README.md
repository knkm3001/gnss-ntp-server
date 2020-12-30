# gnss-ntp-server
オレオレNTPサーバ。
GNSS受信機を用いたStratum1のNTPサーバ。NTPプロトコルをしゃべることができる。   
ぶっちゃけローカルにNTPサーバを立てる意味はないが、あまってたGNSS受信機の活用とプロトコルベースでの実装の練習がメイン。  

## 回路
こちら画像を参考に
![kairo](https://user-images.githubusercontent.com/54818379/103334818-347c5980-4ab6-11eb-9495-a569ea9c970e.png)


## 部品

|  部品名  | 製品名   |
| ---- | ---- |
|  マイコン  |  ESP8266  |
|  GNSS受信機  |  AE-GYSFDMAXB  |
|  ジャンパワイヤ  |  適当なのを複数本  |

## 精度
現状はPPSを使ってないので精度は±0.8secくらい。
NICT(情報通信研究機構 NTP ip:133.243.238.244)と比較すると精度は非常に悪い。
以下画像を参考。
![seido](https://user-images.githubusercontent.com/54818379/103334660-8d97bd80-4ab5-11eb-9c6a-0cf0fb756355.png)

## TODO
- PPSを用いて受信機とESPの時刻同期を行い精度向上させる。
- 現状とりあえずGPSのみの電波を拾っているので"みちびき"にも対応させる。
- ソースコードもっといい感じにする。


