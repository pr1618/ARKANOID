# ARKANOID

・プログラムの概要

    wikipediaとyoutubeをもとにC言語でARKANOIDを再現した。
    スコアシステムや、ゲーム開始と終了のイベントや、音楽や、細かい箇所などは省略。
    パドルやボールの動き、ブロックの破壊、アイテムの落下などを実装。
    
・ソースコード

	  arkanoidSource.cpp
    
・開発環境

	  Visual Studio 2019
    
・開発に使用したライブラリ

	freeglutと、OpenGL Mathematics(GLM)を使用。
  
・操作方法

	マウスを画面上で動かせばパドルが連動して動く。アイテムを取得すれば、左クリックで銃撃や、ドアの開閉などができる。
  
・注意点

	freeglutは64bitバイナリなのでプロジェクトの方も64bitにする。
