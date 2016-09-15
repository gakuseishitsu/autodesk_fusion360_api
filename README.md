## Fusion 360 API を用いてC++でスケッチ描画を行う (2016年9月時点)  

## Abstract
* これまで複雑な図形を描画するのはFusion360のUIからでは難しく. コマンドを用いてプログラムから描画可能なAutoCADで描画したのちdxfデータをFusion360に送信するという手間がかかっていた.  
* しかし最近Fusion360 APIというスクリプトでUIの代わりにスケッチの描画や押し出し等を行えると知りその手順とサンプルコードを自分用にまとめたいと思ったためこのページを作成した.  
* ついでに詳しくなったら時計のガンギ車の自動生成スクリプトとか書いてみたい.  

## How to Start
* 参考文献[1]のページに埋め込まれている1時間くらいの英語のプレゼンを聞けばひと通り流れが分かる.
1.Fusion360を起動する.  
2.ツールボックスのなかの「ADD-INS」を開く.
3.するとでてくるウィンドウの中に「Scripts」や「add-Ins」と書かれたタブが見えるので, 「Scripts」タグの中の「My Scripts」フォルダを選択して「Create」ボタンを押す.  
4.新しく出たウィンドウで設定を行い「Create」ボタンを押す. 設定することは少なく「Scripts」か「add-ins」か, 言語はどれにするか(自分は慣れているC++), あとはスクリプトの名前.  
5.すると「My Scripts」フォルダの中に先ほど作成したスクリプトができるので「Edit」ボタンで編集して「Run」ボタンで実行できる. 何もせずに実行すると簡単なメッセージウィンドウが出てきて終わり.  
6.「Edit」を押すとそれぞれの言語に対応した開発環境が起動してあとはプログラミングするだけ, 簡単なスケッチを各コード※をこのプロジェクトの「src」フォルダにいれてある.  
7.もっといろんな機能が使いたかったら参考文献[2]をみるといい.  

※ スケッチを開いて直線, 円, スプライン曲線を描くプログラム(test1_CPP.cpp)

## References
* Fusion360 APIの初めかたについて書かれているサイト  
<a href="http://autodeskfusion360.github.io/#section_welcome">[1] Autodesk Fusion 360 API</a>
* Fusion360 APIのユーザリファレンスサイト  
<a href="http://fusion360.autodesk.com/learning/learning.html?guid=GUID-A92A4B10-3781-4925-94C6-47DA85A4F65A">[2] Welcome to Fusion's Programming Interface</a>