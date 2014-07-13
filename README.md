# string-splitter

ファイルまたは標準入力のプレーンテキストを全体文字列に対する正規表現フィルタ、NFKC正規化、アルファベットの大文字小文字変換、分かち書き、トークンごとの正規表現フィルタ、英語の活用形を基本形に戻したりして整形します。

## 使い方
コンパイルします。

```bash
% git clone https://github.com/naoa/string-splitter.git
% cd string-splitter
% make
```

標準入力またはファイルからプレーンテキストを読み取り、正規表現フィルタ、NFKC正規化、分かち書きを実行します。

```bash
% echo "Hello! ２０１４年７月１３日は、<b>雨</b>でしょう。" | ./string-splitter
hello 2014 年 7 月 13 日 は 、 雨 でしょ う 。
```

* 入力形式  
UTF8の文字コードのテキストのみ対応しています。

| 引数        | 説明       |デフォルト   |
|:-----------|:------------|:------------|
| --input | 整形対象ファイル名 指定なしで標準入力 標準入力の場合EOSで終了|標準入力|
| --pre_filter |入力テキストから除去したい文字列の正規表現(全置換)。エスケープに注意。|<>タグ除去、改行コード除去、一部の記号(\\,.;:&^/-#'"()[]{}])を除去|
| --no_normalize |NFKC正規化+アルファベットの大文字小文字変換をしない||
| --no_tokenize |分かち書きをしない||
| --mecab_dic |MeCabの辞書を指定できる||
| --use_baseform |MeCabで日本語の活用形を基本形に戻す||
| --token_filter |分かち書き後のトークンから除去したい文字列の正規表現(全置換)||
| --use_wordnet |WordNetを使って英語の活用形を基本形に戻す||
| --cut_prolong |3文字以上のトークンの場合、末尾の長音記号(ー、ｰ)を除去する||
| --h |オプションの説明||

* 出力結果  
整形結果のテキストが標準出力に出力されます。

## 依存関係
このプログラムでは、<code>ICU</code>、<code>MeCab</code>、<code>WordNet</code>、<code>gflags</code>のライブラリを利用しています。

CentOSではたとえば、以下のようにしてインストールできます。

```
% yum install -y icu libicu-devel
% rpm --import http://ftp.riken.jp/Linux/fedora/epel/RPM-GPG-KEY-EPEL
% yum localinstall -y http://ftp-srv2.kddilabs.jp/Linux/distributions/fedora/epel/6/x86_64/epel-release-6-8.noarch.rpm
% yum install -y re2 re2-devel
% yum install -y gflags gflags-devel
% yum install -y wordnet wordnet-devel glib2 glib2-devel
% wget http://mecab.googlecode.com/files/mecab-0.996.tar.gz
% tar -xzf mecab-0.996.tar.gz
% cd mecab-0.996; ./configure --enable-utf8-only; make; make install; 
% echo "/usr/local/lib" > /etc/ld.so.conf.d/mecab.conf
% ldconfig
% wget http://mecab.googlecode.com/files/mecab-ipadic-2.7.0-20070801.tar.gz
% tar -xzf mecab-ipadic-2.7.0-20070801.tar.gz
% cd mecab-ipadic-2.7.0-20070801; ./configure --with-charset=utf8; make; make install
% echo "dicdir = /usr/local/lib/mecab/dic/ipadic" > /usr/local/etc/mecabrc
```

## Author

Naoya Murakami naoya@createfield.com

## License

MIT License 

