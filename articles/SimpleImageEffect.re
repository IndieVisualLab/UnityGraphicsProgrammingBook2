= ImageEffect 入門


//image[SimpleImageEffect/02mono][ImageEffect によるネガポジ反転]


出力される映像に対してシェーダ（GPU）を使ってエフェクトを適用する技術
ImageEffect（イメージエフェクト）を Unity で実現する方法についてシンプルに解説します。
同技術は PostEffect（ポストエフェクト）とも呼ばれています。

ImageEffect は光を表現するグロー効果、ジャギーを軽減するアンチエイリアシング、
被写界深度 DOF の実現、その他、のあらゆる演出のために用いられています。
もっとも簡単な例は、ここで紹介するサンプルも扱うような色の変更や修正でしょう。

本章では Unlit シェーダや、Surface シェーダについての基本的な知識や使い方について、
多少の前提知識があることを前提に書いていますが、もっとも簡単な構成のシェーダであるため、
前提知識がなくても、読み進めて使うことはできると思います。

本章のサンプルは@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming2}@<br>{}
の「SimpleImageEffect」です。

== ImageEffect の働き


ImageEffect がどのようにしてさまざまな効果を実現しているのかというと、
簡潔に述べれば画像処理、すなわち画面を画素単位で操作することによって、さまざまな効果を実現しています。

シェーダによって画素を処理するといえば、フラグメントシェーダです。
実質的に、ImageEffect を実装することはフラグメントシェーダを実装することに等しいといえます。

//image[SimpleImageEffect/01][ImageEffect の実装はフラグメントシェーダの実装]


== Unity における ImageEffect の簡単な流れ


Unity において、ImageEffect の処理順序は概ね次のようになります。

 1. カメラがシーンを描画する。
 2. カメラの描画内容が OnRenderImage メソッドに入力される。
 3. ImageEffect 用のシェーダで、入力された描画内容を修正する。
 4. OnRenderImage メソッドが修正された描画内容を出力する。


== サンプルシーンから構成を確認する


もっとも簡単なサンプルシーンを用意しました。サンプルの "ImageEffectBase" シーンを開いて確認してください。
関連するスクリプトなどのリソースは、それと同じ名前のものです。

よく似たサンプルに ImageEffect シーンと、同じ名前のリソースがありますが、そちらは後で解説するので注意してください。

サンプルを開くと、シーン中のカメラが映し出すイメージは、ImageEffect によって色がネガポジ反転します。
これは Unity が標準で生成する ImageEffect 用のシェーダと同等の効果ですが、実際のソースコードはわずかに異なります。

サンプルシーンの "Main Camera" に "ImageEffectBase" スクリプトがアタッチされていることを確認してください。
さらに "ImageEffectBase" では同じ名前のマテリアルが参照され、そのマテリアルには同じ名前のシェーダが設定されています。


== スクリプトの実装


まずは ImageEffect のシェーダをスクリプトから呼び出すまでの処理を先に解説しようと思います。


=== OnRenderImage メソッド


Unity が出力する映像に変更を加えたいとき、ほとんどの場合には OnRenderImage メソッドを実装する必要があります。
OnRenderImage は Start や Update などと同じように Unity の標準のワークフローに定義されるメソッドです。

//emlist[ImageEffectBase.cs]{
[ExecuteInEditMode]
[RequireComponent(typeof(Camera))]
public class ImageEffectBase : MonoBehaviour
{
…
protected virtual void OnRenderImage
    (RenderTexture source, RenderTexture destination)
{
    Graphics.Blit(source, destination, this.material);
}
//}

OnRenderImage は、Camera コンポーネントをもつ GameObject に追加されたときにだけ呼び出されます。
したがって、ImageEffect クラスには、@<code>{[RequireComponent(typeof(Camera))]} を定義しています。

また ImageEffect が適用された結果は、Scene を実行する前から見えていた方がよいので、@<code>{ExcludeInEditMode} 属性も定義されています。
複数の ImageEffect を切り替えて確認するときや、無効にした状態を確認するときは、ImageEffect のスクリプトを disable にします。


==== 引数 source, destination について


OnRenderImage は引数の 1 つ目（source）に入力、2 つ目（destination）に出力先が与えられています。
いずれも RenderTexture 型ですが、特別の設定がないとき、source にはカメラの描画結果が与えられ、destination には null が与えられています。

ImageEffect は、source に入力された絵を変更して destination に書き込む処理を行いますが、
destination が null のとき、変更された絵の出力先はフレームバッファ、すなわちディスプレイに見えている領域になります。

また Camera の出力先に RenderTexture が設定されているとき、source はその RenderTexture と同等になります。


==== Graphics.Blit


@<code>{Graphics.Blit} メソッドは、指定したマテリアルとシェーダを使って、
入力された RenderTexture を出力となる RenderTexture に描画する処理です。
ここでの入力と出力は OnRenderImage の source および destination になります。
また、マテリアルは ImageEffect のためのシェーダが設定されたものになります。

原則として、 OnRenderImage メソッドでは、引数の destination に必ず何かしらのイメージデータを渡す必要があります。
したがってほとんどの場合に OnRenderImage 内では Graphics.Blit が呼び出されます。

応用的に、たとえば別のエフェクトに使うためのテクスチャを作るとき、
テクスチャを複製するときなどにも @<code>{Graphics.Blit} を用いることがあります。
あるいは、別の方法を用いて destination にデータを渡すことがありますが、
ここでは入門のためにそれらの応用例については割愛します。

次の項目は ImageEffect を適用する過程の話と少々異なるので、
もし、はじめて読まれるときは、飛ばしてシェーダの解説に進むことを推奨します。


=== ImageEffect が利用可能か検証する


ImageEffect を解説するにあたり、この項目の実装および解説は不要と思うのですが、
より実用的な実装がなされた資料を読むときに障害にならないよう、解説することにしました。
Unity が用意する ImageEffect の資料にも同等の機能が実装されています。

ImageEffect は画素毎に演算される処理です。
したがって高度な GPU を持たない実行環境では、演算数の多さのために　ImageEffect が歓迎されないことがあります。
そこで、ImageEffect が実行環境で利用できるかどうか、開始時に検証し、利用できなければ無効化するのが親切です。

//emlist[ImageEffectBase.cs]{
protected virtual void Start()
{
    if (!SystemInfo.supportsImageEffects
     || !this.material
     || !this.material.shader.isSupported)
    {
        base.enabled = false;
    }
}
//}

検証は Unity が用意する @<code>{SystemInfo.supportsImageEffects} によって簡単に実現することができます。

この実装はほとんどのケースで有用と思いますが、たとえばシェーダ側に実装される
fallback（フォールバック）機能を使う場合などのときは、異なる実装が必要になることがあります。あくまで参考にしてください。

1 つだけ注意する必要があるとすれば、@<code>{this.material} の参照を検証するタイミングです。
例では Start メソッドで検証していますが、これが Awake や OnEnable のとき、
たとえば @<code>{this.material} に参照が与えられていたとしても、
Unity は null を示します(そして @<code>{base.enabled = false} によってスクリプトが無効になります)。
詳細は割愛しますが、@<code>{ExcludeInEditMode} の仕様に依存するものです（弊害とは言い難い）。


== もっとも簡単な ImageEffect シェーダの実装


続いて ImageEffect のシェーダについて解説します。
ここで紹介するもっとも基本的なサンプルは、Unity が標準的に作成するものと同様に、出力する色を反転するだけの効果を実装しています。


//emlist[ImageEffectBase.shader]{
Shader "ImageEffectBase"
{
    Properties
    {
        _MainTex("Texture", 2D) = "white" {}
    }
    SubShader
    {
        Cull Off ZWrite Off ZTest Always

        Pass
        {
            CGPROGRAM

            #include "UnityCG.cginc"
            #pragma vertex vert_img
            #pragma fragment frag

            sampler2D _MainTex;

            fixed4 frag(v2f_img input) : SV_Target
            {
                float4 color = tex2D(_MainTex, input.uv);
                color.rgb = 1 - color.rgb;

                return color;
            }

            ENDCG
        }
    }
}
//}

大まかな処理の流れとして、@<code>{_MainTex} にカメラが描画するイメージが入力され、
フラグメントシェーダで画素に最終的に表示する色を決定する、といったイメージです。

ここで @<code>{_MainTex} に与えられるテクスチャ情報は @<code>{OnRenderImage} の @<code>{source}, 
@<code>{Graphics.Blit} の @<code>{source} に等しいです。

@<code>{_MainTex} は @<code>{Graphics.Blit} の入力のために Unity によって予約されている点に注意してください。
異なる他の名前に変更すると、@<code>{Graphics.Blit} の @<code>{source} が正しくシェーダに入力されません。


=== Unity が標準で生成するシェーダとの差異


Unity が標準で生成する ImageEffect は次のように少々長く複雑です（抜粋しています）。
ImageEffect もシェーダですから、標準的なレンダリングパイプラインを経て最終的な出力を得ます。
したがって、ImageEffect が実現する効果には本質的に影響しないように見えるバーテックスシェーダも、
ImageEffect のシェーダには定義されている必要があります。

//emlist[NewImageEffectShader.shader]{
SubShader
{
    Cull Off ZWrite Off ZTest Always

    Pass
    {
        CGPROGRAM
        #pragma vertex vert
        #pragma fragment frag

        #include "UnityCG.cginc"

        struct appdata
        {
            float4 vertex : POSITION;
            float2 uv : TEXCOORD0;
        };

        struct v2f
        {
            float2 uv : TEXCOORD0;
            float4 vertex : SV_POSITION;
        };

        v2f vert (appdata v)
        {
            v2f o;
            o.vertex = UnityObjectToClipPos(v.vertex);
            o.uv = v.uv;
            return o;
        }

        sampler2D _MainTex;

        fixed4 frag (v2f i) : SV_Target
        {
            fixed4 col = tex2D(_MainTex, i.uv);
            col.rgb = 1 - col.rgb;
            return col;
        }
        ENDCG
    }
}
//}

ImageEffect におけるバーテックスシェーダは、カメラに正面を向いて、
その全面を埋めるような四角形のメッシュとその UV 座標を、フラグメントシェーダに渡しているだけです。
このバーテックスシェーダに手を加えることで実現できる効果もありますが、ほとんどの ImageEffect では不要です。

そのためか、Unity には標準的なバーテックスシェーダとその入力を定義するための構造体が用意されています。
それらは "UnityCG.cginc" に定義されています。ここで用意した標準ではないシェーダのソースコードでは、
UnityCg.cginc に定義された @<code>{vertex vert_img} や @<code>{appdata}、@<code>{v2f_img} 
を利用することによって、ソースコード全体を簡潔にしています。


=== Cull, ZWrite, ZTest


一見するとカリングと Zバッファの書き込み、参照については、標準の値で問題なさそうです。
しかしながら Unity は不用意なＺバッファへの書き込みを防ぐために、
@<code>{Cull Off ZWrite Off ZTest Always} の定義を推奨しています。


== もっとも簡単な練習


ImageEffect を簡単に練習してみましょう。サンプルでは単純に全画面をネガポジ反転するようにしていますが、
この章の初めに掲載した図のように、イメージ全体の "斜め半分にだけ" ネガポジ反転を適用するようにしてみます。

@<code>{input.uv} には、イメージ全体のうち 1 画素分を示す座標が与えられていますから、これを活用します。
イメージ全体の各画素は 0 ~ 1 で正規化された x * y 座標で示されます。

動作する一例のコードは、サンプルの "Prtactice" フォルダに含まれていますし、後に続いて解説しますが、
もし初心者の方に目を通して頂けているのであれば、まずご自身で実装してみることをオススメします。


=== 上下左右の半分は簡単


上下半分に色を変化させるのはすごく簡単です。これによって、ImageEffect の座標の原点を確認するのがよいと思います。
たとえば次の 2 行のコードでは、それぞれ x 座標と y 座標が半分より小さいときに色を反転します。

//emlist[Practice/ImageEffectShader_01.shader]{
color.rgb = input.uv.x < 0.5 ? 1 - color.rgb : color.rgb;
color.rgb = input.uv.y < 0.5 ? 1 - color.rgb : color.rgb;
//}

色の変化から、ImageEffect に与えられる座標の原点は、左下であることが確認できたでしょうか。


=== 斜め半分も簡単


上下左右の半分は簡単だと先にいっていますが、実際には斜め半分も簡単です。
次のようなソースコードで斜め半分にエフェクトを適用する（色を反転する）ことができます。

//emlist[Practice/ImageEffectShader_02.shader]{
color.rgb = input.uv.y < input.uv.x ? 1 - color.rgb : color.rgb;
//}


== 座標に関する便利な定義値


紹介したように UnityCg.cginc には @<code>{vertex vert_img} や 
@<code>{appdata} のような便利な関数や構造体が定義されていますが、
これら以外にも ImageEffect を実装する上で便利な値が定義されています。


=== _ScreenParams


@<code>{_ScreenParams} は、@<code>{float4} 型の値で、
@<code>{x, y} にはそれぞれ出力するイメージのピクセル幅と高さが、
@<code>{w, z} には @<code>{1 + 1 / x}, @<code>{1 + 1 / y} が与えられています。

たとえば 640x480 サイズのレンダリングを実行するとき、@<code>{x = 640}, @<code>{y = 480}, 
@<code>{z = 1 + 1 / 640}, @<code>{z = 1 + 1 / 480} となります。
実際のところ、@<code>{w} と @<code>{z} はそれほど使うことがないでしょう。

一方で @<code>{x}、@<code>{y} の値はたとえばイメージ上の何ピクセルに相当するのか算出したり、
あるいはアスペクト比を算出するために頻繁に用いられます。これらは凝ったエフェクトを作る上で重要ですが、
わざわざスクリプトから値を与えずとも Unity 側が用意してくれるのは助かりますね。
頭の片隅に入れておけば、他の方のシェーダを読むときの助けになることもあると思います。


=== _TexelSize


同じような定義値の 1 つに、@<code>{<sampler2Dの変数名>_TexelSize} という定義値があります。
ここでは @<code>{_MainTex_TexelSize} になります。

@<code>{_ScreenParams} と同じく @<code>{float4} 型の値ですが、
@<code>{x = 1 / width}, @<code>{y = 1 / height}, @<code>{z = width}, @<code>{y = height} と、
各要素に与えられる値が異なります。また対応する @<code>{sampler2D} 型によって値が異なる点も特徴です。
@<code>{_MainTex} にかかわらず、対応する @<code>{~_TexlSize} を定義すれば、Unity から値が与えられます。

@<code>{_ScreenParams} を使っている ImageEffect も沢山ありますが、
どちらかというと @<code>{_MainTex_TexelSize} の方が使いやすいと思います。


=== 1 つとなりの画素を参照する


たとえば 1 つ隣の画素の色（値）を参照したい、ということは画像処理などでは頻繁にありますが、
次のようなコードによって、隣の画素の値を参照することができます。

//emlist[Practice/ImageEffectShader_03.shader]{
sampler2D _MainTex;
float4    _MainTex_TexelSize;

fixed4 frag(v2f_img input) : SV_Target
{
    float4 color = tex2D(_MainTex, input.uv);

    color += tex2D(_MainTex, input.uv + float2(_MainTex_TexelSize.x, 0));
    color += tex2D(_MainTex, input.uv - float2(_MainTex_TexelSize.x, 0));
    color += tex2D(_MainTex, input.uv + float2(0, _MainTex_TexelSize.y));
    color += tex2D(_MainTex, input.uv - float2(0, _MainTex_TexelSize.y));

    color = color / 5;

    return color;
}
//}

このコードは周辺 4 つの画素を参照して平均の値を返すものです。画像処理では文字どおり平滑化フィルタなどと呼ばれます。
他にもより高品質なノイズ軽減のためのフィルタが同じように周辺の画素を参照して実装されることもありますし、
たとえばエッジ・輪郭線検出フィルタなどでも用いられています。


== 深度と法線の取得


//image[SimpleImageEffect/03mono][G-Buffer のイメージ]


モデルに適用するマテリアル（シェーダ）を実装するときは、その多くの場合にモデルの深度や法線情報を参照すると思います。
2 次元のイメージ情報を操作する ImageEffect においては、深度や法線情報を取得できないように思われますが、
イメージ上のある画素に映されるオブジェクトの深度や法線情報を取得する方法は用意されています。

技術的な詳細を解説するにはレンダリングパイプラインの解説が必要になり、少々長くなるので割愛させてください。
簡潔に説明すると、描画するイメージ上のある画素に対応する深度情報や法線情報はバッファしておくことができます。
それらのバッファは G-Buffer と呼ばれます。G-Buffer には色を保存したり、深度を保存したりするものがあります。
（ちなみに G-Buffer の読み方は「ゲーバッファー」であると原著論文には示されています。）

オブジェクトの描画時に、深度や法線情報もバッファに書きこんでおき、
描画の最後に実行される ImageEffect でそれを参照する、というイメージです。
この技術は Deffered レンダリングでは重要な役割を持っていますが、Forward レンダリングでも使うことができます。

これらの解説にはサンプルの "ImageEffect" シーンおよび、それと同名のリソースを使います。


=== 深度と法線情報を取得するための設定


深度と法線情報を ImageEffect で参照するためには、少し設定が必要です。
基本的な機能は共通なので、ここでは ImageEffectBase.cs を継承した ImageEffect.cs でその設定を行うようにします。

//emlist[ImageEffect.cs]{
public class ImageEffect : ImageEffectBase
{
    protected new Camera camera;
    public DepthTextureMode depthTextureMode;

    protected override void Start()
    {
        base.Start();

        this.camera = base.GetComponent<Camera>();
        this.camera.depthTextureMode = this.depthTextureMode;
    }

    protected virtual void OnValidate()
    {
        if (this.camera != null)
        {
            this.camera.depthTextureMode = this.depthTextureMode;
        }
    }
}
//}

深度と法線情報を取得するためには、カメラに @<code>{DepthTextureMode} を設定する必要があります。
これは深度や法線などの情報を、どのように書き込むかを制御するための設定です。初期値は @<code>{None}です。

残念ながら @<code>{DepthTextureMode} はカメラの Inspector に表示されないパラメータなので、
スクリプトから任意にカメラの参照を取得して設定する必要があります。

@<code>{OnValidate} メソッドについて、あまり利用したことがない方のために説明しておくと、
Inspector 上でパラメータが更新されたときに呼び出されるメソッドです。


=== DepthTextureMode の値


ここで紹介するコードを使って、@<code>{DepthTextureMode} の値を Inspector 上から変更します。
いくつか値がありますが、ここでは @<code>{DepthNormals} を使う点に注意してください。

@<code>{Depth} を設定すれば深度情報のみを取得するための設定になります。
ただし @<code>{Depth} と @<code>{DepthNormals} とでは、シェーダから深度情報を取得する手順が少々異なります。
また @<code>{MotionVectors} を設定すれば、各画素に対応する動きの情報を取得することができ大変面白いのですが、
すべて解説すると少々長くなるので、この場では割愛させてください。


== シェーダ上での深度と法線の取得


カメラに @<code>{DepthTextureMode} を設定したとき、シェーダ上から深度情報と法線情報を取得する方法は次のとおりです。

@<code>{_CameraDepthNormalsTexture} は、@<code>{_MainTex} に描画するイメージが与えられるのと同様に、
深度と法線の情報が与えられる @<code>{sampler2D} です。したがって @<code>{input.uv} を使って参照すれば、
描画するイメージのある画素に対応する深度と法線情報を取得することができます。

//emlist[ImageEffect.shader]{
sampler2D _MainTex;
sampler2D _CameraDepthNormalsTexture;

fixed4 frag(v2f_img input) : SV_Target
{
    float4 color = tex2D(_MainTex, input.uv);
    float3 normal;
    float  depth;

    DecodeDepthNormal
    (tex2D(_CameraDepthNormalsTexture, input.uv), depth, normal);

    depth = Linear01Depth(depth);
    return fixed4(depth, depth, depth, 1);

    return fixed4(normal.xyz, 1);
}
//}

@<code>{_CameraDepthNormalsTexture} から取得することができる値は深度と法線の値が合わさったものなので、
これをそれぞれの値に分解する必要があります。分解するための関数は Unity が用意してくれているものを使います。
@<code>{DecodeDepthNormal} 関数に、分解したい値とその結果を代入するための変数を与えます。


=== 深度情報の取得と可視化

//image[SimpleImageEffect/04mono][ImageEffect による深度の可視化]

先に深度情報について説明します。深度情報は実はプラットフォームによって扱いが異なります。
Unity ではその差を吸収するためのいくつかの仕組みが用意されていますが、
ImageEffect の実装にあたっては、@<code>{Linear01Depth} 関数を使うのがよいと思います。
@<code>{Linear01Depth} は、取得した深度の値を 0 ~ 1 に正規化するための関数です。

サンプルでは取得した深度の値を R,G,B に与えることで、深度の値を可視化しています。
シーン中のカメラを動かしたり、@<code>{Clipping Planes} の値を Inspector 上から変更するなどして、
どのように変化するのかを確認することをオススメします。


=== 法線情報の可視化

//image[SimpleImageEffect/05mono][ImageEffect による法線の可視化]

法線情報の可視化については、深度情報ほどの複雑さはありません。
法線の情報はスクリプトや一般的なシェーダから参照されるものと同等です。
ある画素に映される面の方向を示す X,Y Z の情報が 0 ~ 1 に正規化された形式で与えられています。

法線が正しく取得できているかどうかを確認するだけなら、X,Y,Z の値をそのまま R,G,B とみなして出力すればよいです。
つまり右を向く面ほど X = R の値が大きくなり、より赤に、上を向く面ほど Y = G の値が大きくなり、より緑になります。


== 参考


本章での主な参考資料は次のとおりです。いずれも Unity 公式のものです。

 * Writing Image Effects- @<href>{https://docs.unity3d.com/540/Documentation/Manual/WritingImageEffects.html}
 * Accessing shader properties in Cg/HLSL - @<href>{https://docs.unity3d.com/Manual/SL-PropertiesInPrograms.html}
 * Using Depth Textures - @<href>{https://docs.unity3d.com/ja/current/Manual/SL-DepthTextures.html}
