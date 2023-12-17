# Animepoy

**アニメっぽいエフェクト集**

Unreal Engine 5 でアニメっぽいエフェクトを追加するためのプラグインです。


![Screenshot](https://github.com/Pachira762/Animepoy/assets/34003267/9360a5c0-2914-4b6c-b052-c4dad54f0652)
![Screenshot Line](https://github.com/Pachira762/Animepoy/assets/34003267/49ef3619-1c15-4f69-81f6-4a70b65d06a0)

## 機能

* ラインアート
    * ポストプロセスベースのラインエフェクト
    * DoF や Fog と調和
    * シーンカラー / ベースカラー への描画

* Kuwahara フィルター
    * シーンカラー、ベースカラー、ワールド法線、Metallic Specular Roughness へのフィルター
    * ディフュージョンフィルター

## 前提条件

* HLSL Shader Model 6.5 に対応した環境

## プラグインのインストール方法

[Releases](https://github.com/Pachira762/Animepoy/releases/latest) からプラグインをダウンロードし、`Animepoy`フォルダをプロジェクトの`Plugins`フォルダにコピーしてください。

## プラグインの使用方法

1. プロジェクトの設定から `デフォルトのRHI` を `DirectX 12` に、
    `D3D12 Targeted Shader Formats` を `SM6` に設定します。
2. コンテンツブラウザから `Plugins\Animepoy C++クラス\Animepoy\Public\AnimepoySettings` を選び、レベルに配置してください。
3. AnimepoySettings アクタの詳細からエフェクトの設定を行います。
4. ディフュージョンフィルターのブラーの半径を大きくする場合 `r.Filter.LoopMode` を `1` に設定します。

## 注意事項

* KuwaharaFilter シェーダーに Wave Intrinsic を使用しています。
* HLSL Shader Model 6.5 に対応していない環境では、KuwaharaFilter が正しく動作しません。

## ライセンス

MIT License

## 連絡
[@pachira762](https://twitter.com/pachira762)
