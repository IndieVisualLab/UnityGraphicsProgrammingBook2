= Curl Noise - 疑似流体のためのノイズアルゴリズムの解説

== はじめに
本章では、擬似流体アルゴリズムであるCurl Noise（カールノイズ）のGPU実装についての解説を行なっていきます。@<br>{}
本章のサンプルは@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming2}@<br>{}
の「CurlNoise」です。

=== Curl Noiseとは
Curl Noiseとは、FLIP法等の流体アルゴリズムの開発者としても知られているブリティッシュ・コロンビア大学のRobert Bridson教授が、2007年に発表した擬似流体ノイズアルゴリズムです。@<br>{}
前作の「Unity Graphics Programming vol.1」にて、ナビエ・ストークス方程式を使った流体シミュレーションの解説をさせて頂きましたが、Curl Noiseはそれらの流体シミュレーションと比べて擬似的ではありますが軽負荷にて流体表現をすることが可能です。@<br>{}
特に昨今、ディスプレイやプロジェクターの技術躍進に伴い4Kや8K等の高解像においてリアルタイムレンダリングを行う必要性が高まっていますので、Curl Noiseの様な低負荷なアルゴリズムは、流体表現を高解像度や低マシンスペックで表現する為の有用な選択肢となります。

== Curl Noiseのアルゴリズム
流体シミュレーションにおいて、まず必要となるものが「速度場」と呼ばれるベクトル場です。まずは速度場というものがどういう物かを図でイメージしてみましょう。@<br>{}
以下が2次元の場合の速度場のイメージ図です。平面上の各点において、ベクトルが定義されているのが見て取れるかと思います。@<br>{}

//image[VectorField2d][二次元上での速度場の観測][scale=0.7]{
//}

上図の様に、平面上の各微分区間において、それぞれベクトルが個別に定義されている状態をベクトル場といい、それぞれのベクトルが速度である物を速度場と言います。@<br>{}
これらは3次元上であっても、立方体の中の各微分ブロックにおいてベクトルが定義されている状態と想像していただけると、解りやすいかと思います。@<br>{}
それでは、Curl Noiseはどの様にして、この速度場を導出しているのかを見ていきます。

Curl Noiseの面白いところは、前章の「プロシージャルノイズ入門」でも解説されたPerlin NoiseやSimplex Noise等の勾配ノイズをポテンシャル場として用い、そこから流体の速度場を導出するところにあります。@<br>{}
本章ではポテンシャル場として、3次元Simplex Noiseを利用する事とします。@<br>{}

以下、まずはCurl Noiseの数式からアルゴリズムを紐解いていきたいと思います。

//texequation{
\overrightarrow{u} = \nabla \times \psi
//}

上記はCurl Noiseのアルゴリズムになります。@<br>{}
左辺@<m>{\overrightarrow{u\}}は導出された速度ベクトル、右辺@<m>{\nabla}はベクトル微分演算子（ナブラと読み、偏微分の作用素として働きます）、@<m>{\psi}はポテンシャル場です。（本章では3次元Simplex Noise）@<br>{}
Curl Noiseはこの右辺二項の外積を取ったものとして表せます。@<br>{}

つまり、Curl NoiseはSimplex Noiseとベクトル各要素の偏微分@<m>{ \left( \dfrac {\partial \} {\partial x\}_, \dfrac {\partial \} {\partial y\}_, \dfrac {\partial \} {\partial z\} \right) }の外積をとったものであり、過去にベクトル解析を学ばれた事がある方にとっては、rotA（回転）の形そのものである事が
見て取れるかと思います。@<br>{}
それでは、3D Simplex Noiseと偏微分の外積を計算してみましょう

//texequation{
\overrightarrow{u} = \left( \dfrac {\partial \psi _3} {\partial y} - \dfrac {\partial \psi _2} {\partial z}_, \dfrac {\partial \psi _1} {\partial z} - \dfrac {\partial \psi _3} {\partial x}_,  \dfrac {\partial \psi _2} {\partial x} - \dfrac {\partial \psi _1} {\partial y} \right)
//}

一般的に外積は、二つのベクトルのお互いにとっての垂直方向を向き、その長さは両ベクトルで張られる面の面積と同じになるという特徴を持ったものですが、
ベクトル解析でのrotA（回転）での外積演算のイメージの掴み方は、「ねじれた各偏微分要素方向のポテンシャル場のベクトルをルックアップし、その項同士を引いているから回転が起きる」と、上記の計算式からシンプルに捉えた方がイメージを掴みやすいかもしれません。@<br>{}
実装自体はとても単純で、偏微分の各要素方向に微小にルックアップポイントをずらしながら、上記@<m>{\psi}の各点、つまり3次元SimplexNoiseからベクトルをルックアップし、上記の数式の様に、外積演算をするだけです。

=== 質量保存則について

もし前作の「Unity Graphics Programming vol.1」の流体シミュレーションの章をお読みになられた方は、質量保存則はどうなっているのかと気になられた方もいらっしゃるかもしれません。@<br>{}
質量保存則とは、流体は速度場での各ポイントにおいて、必ず、流入と流出の均衡がとれ、流入した分流出し、流出した分は流入してきており、最終的には発散ゼロ（ダイバージェンスフリー）というルールでした。

//texequation{
\nabla \cdot \overrightarrow{u} = 0
//}

この事については、論文でも述べられているのですが、そもそも勾配ノイズ自体がなだらかに変化している物ですので（2次元グラデーションでイメージするなら、左側のピクセルが薄ければ、右側は濃くなっている様に）、ダイバージェンスフリーはポテンシャル場の時点で保証されているという物でした。パーリンノイズの特徴から考えてみれば至極当然な事ですね。

=== Curl Noiseの実装

それでは、数式に基づいて、Compute shader、若しくはShaderでCurlNoise関数をGPU実装してみましょう。

//emlist{
#define EPSILON 1e-3

float3 CurlNoise(float3 coord)
{
    float3 dx = float3(EPSILON, 0.0, 0.0);
    float3 dy = float3(0.0, EPSILON, 0.0);
    float3 dz = float3(0.0, 0.0, EPSILON);

    float3 dpdx0 = snoise(coord - dx);
    float3 dpdx1 = snoise(coord + dx);
    float3 dpdy0 = snoise(coord - dy);
    float3 dpdy1 = snoise(coord + dy);
    float3 dpdz0 = snoise(coord - dz);
    float3 dpdz1 = snoise(coord + dz);

    float x = dpdy1.z - dpdy0.z + dpdz1.y - dpdz0.y;
    float y = dpdz1.x - dpdz0.x + dpdx1.z - dpdx0.z;
    float z = dpdx1.y - dpdx0.y + dpdy1.x - dpdy0.x;

    return float3(x, y, z) / EPSILON * 2.0;
}
//}

上記の様に、このアルゴリズムはシンプルな四則演算の形に落とし込む事ができますので、実装自体はとても簡単で、これだけの行で実装ができてしまいます。@<br>{}
以下に今回コンピュートシェーダにて実装したCurl Noiseのサンプルを貼っておきます。パーティクルの粒を移流たり、上昇ベクトルを加えて炎の様に見せたり、アイデア次第で様々な表情を引き出す事が可能です。

//image[CurlNoise1][][scale=0.71]{
//}
//image[CurlNoise2][][scale=0.71]{
//}
//image[CurlNoise3][][scale=0.71]{
//}

== まとめ
本章ではCurl Noiseによる擬似流体の実装について解説しました。@<br>{}
少ない負荷と実装で3次元の擬似流体を再現可能な為、高解像度でのリアルタイムレンダリングにおいては特に有用に働くアルゴリズムではないでしょうか。

まとめとして、Curl Noiseアルゴリズムをはじめ、様々な技法を今もあみ出されているRobert Bridson教授への最大限の謝辞を込めて、この章のまとめとさせていただきたいと思います。@<br>{}
説明が至らない点、わかりづらい部分もあったかと思いますが、ぜひ読者の方々にもグラフィックスプログラミングを楽しんで頂けますと幸いです。

== References

 * Robert Bridson, Jim Hourihan, Marcus Nordenstam. 2007, Curl-noise for procedural fluid flow. In proc, ACM SIGGRAPH 46.
