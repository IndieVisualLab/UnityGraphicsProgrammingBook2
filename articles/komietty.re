= ImageEffect応用（SSR）

== はじめに

本章ではImageEffectの応用として、Screen Space Reflection の理論と実装について解説します。
３次元空間を構成する際に、映り込みや反射は陰影と並んでリアリティを表現するために役立つものです。
しかしながら、反射や映り込みは、我々が日常で目にする現象の単純さにも関わらず、３DCGの世界でレイトレーシング（後述）などを用いて物理現象に忠実に再現しようとすると、膨大な演算量が必要とされる表現でもあります。
最近ではUnityでOctanRendererが使えるようになり、映像作品として制作する場合はUnityでもかなりフォトリアルな演出が可能になってきてはいますが、リアルタイムレンダリングではまだまだ工夫を凝らして擬似的に再現する必要があります。

リアルタイムレンダリングで反射を表現するテクニックはいくつか存在しますが、本章ではScreen Space Reflection (以下SSR) という、ポストエフェクトに属する手法を紹介していきます。

本章の構成としては、先ずポストエフェクトの肩慣らしとして、サンプルプログラム中でも使用されているブラー処理について先取りして解説していきます。
その後、SSRについて、できるだけ小さな処理の単位に分解しながら、解説していきます。

また、本章のサンプルは@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming2}@<br>{}
の「SSR」に入っています。

== Blur

本節ではブラー処理について解説していきます。アンチエイリアシングまで含めると、ブラー処理も大変ややこしい手続きを理解する必要がありますが、今回は肩慣らしなので基本的な処理です。
ブラー処理の基本は、処理を施したい画像の各テクセル（ラスタライズされた後のピクセルのこと@<fn>{texel}）に対して、その周辺のテクセルを参照する行列を掛け合わせることで、テクセルの色を均質化していきます。
この周辺のテクセルを参照する行列をカーネル（核）と呼びます。カーネルはテクセルの色を混ぜ合わせる割合を決める行列といえます。

ブラー処理のなかでもっともよく使われるのがガウシアンブラーです。これは名前の通り、カーネルにガウス分布を利用する処理を指します。
ガウシアンブラーの実装を斜め読みしながら、ポストエフェクトにおける処理の感覚を掴んでいきましょう。

ガウシアンカーネルは処理対象の画素を中心に、ガウス分布に従う割合で輝度を混ぜ合わせます。こうすることで輝度が非線形に変わる輪郭部分のボケを抑えることができます。

数学の復習になりますが、ガウス分布は以下の式で表すことができます。

//texequation{
G\left( x\right) =\dfrac {1}{\sqrt {2 \pi \sigma ^{2}}}\exp \left( -\dfrac {x^{2}}{2\sigma ^{2}}\right)
//}

ここでガウス分布は二項分布に近似できるので、以下のように二項分布に従う重み付けの組み合わせでガウス分布の代用ができます（ガウス分布と二項分布の近似については脚注参照@<fn>{binomial}）

//emlist[GaussianBlur.shader]{
float4 x_blur (v2f i) : SV_Target
{
  float weight [5] = { 0.2270270, 0.1945945, 0.1216216, 0.0540540, 0.0162162 };
  float offset [5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };
  float2 size = _MainTex_TexelSize;
  fixed4 col = tex2D(_MainTex, i.uv) * weight[0];
  for(int j=1; j<5; j++)
  {
    col += tex2D(_MainTex, i.uv + float2(offset[j], 0) * size) * weight[j];
    col += tex2D(_MainTex, i.uv - float2(offset[j], 0) * size) * weight[j];
  }
  return col;
}
//}

上記のコードはx方向のみですが、y方向もほぼ同様の処理になります。
ここでｘ方向とｙ方向のブラーを分けているのは、２方向に分割することで、輝度取得の回数を @<m>{n * n = n ^ 2} 回から @<m>{n * 2 + 1= 2n + 1} 回に縮減できるからです。

//image[komietty/ssr_blur][各方向のBlurの合成で正しくブラーがかかることの確認]{
//}

スクリプト側では@<code>{OnRenderImage}でxyそれぞれの方向についてsrcと一時的なRenderTextureの間を交互にBlitし、最後ににsrcからdstへBlitして出力しています。
MacOSではsrcだけでBlitが可能でしたが、Windowsでは結果が出力されなかったため、@<code>{RenderTexture.GetTemporary}を使用しています。
（OnRenderImageとBlitについては前章のImageEffect入門を参照して下さい。）

//emlist[GaussianBlur.cs]{
void OnRenderImage(RenderTexture src, RenderTexture dst)
{
  var rt = RenderTexture.GetTemporary(src.width, src.height, 0, src.format);

  for (int i = 0; i < blurNum; i++)
  {
    Graphics.Blit(src, rt, mat, 0);
    Graphics.Blit(rt, src, mat, 1);
  }
  Graphics.Blit(src, dst);

  RenderTexture.ReleaseTemporary(rt);
}
//}

以上でガウシアンブラーの解説は終わりです。
ポストエフェクトがどのように行われるかの感覚が掴めてきたかと思いますので、次節からSSRの解説を行います。

== SSR

SSRはポストエフェクトの範囲内で反射・映り込みを再現しようとするテクニックです。
SSRに必要なのはカメラで撮られた画そのものと、深度情報が書き込まれたデプスバッファ、あとは法線情報が書き込まれたノーマルバッファです。
デプスバッファやノーマルバッファなどはG-bufferと総称されるもので、SSRのようなDeferredレンダリングにとっては必要不可欠なものとなります。
（Deferredレンダリングについては前章のImageEffect入門に素晴らしい解説がありますので、ぜひそちらを参照してください。）

本節を読む際の前提ですが、本節ではレイトレーシングについての基本的な知識を前提にして解説を進めていきます。
レイトレーシングについては、入門レベルでも別にもう一章書けるくらい大きなテーマなので、残念ながらここでは説明は割愛させていただきます。
ただ、レイトレーシングが何か分からないと以下の内容も理解できないので、分からないという方はPeterShirleyの良入門書 "Ray Tracing in One Weekend"@<fn>{shirley}がありますので、先ずそちらを読まれることをおすすめします。

また、SSRのUnity実装での解説テキストとしてはkode80の「Screen Space Reflections in Unity 5 @<fn>{kode80}」が有名です。また日本語のテキストでは「Unity で Screen Space Reflection の実装をしてみた @<fn>{hecomi}」があります。
本節では上記のテキストで解説されていることは、極力解説を簡略化したり、枝葉のテクニックについては解説を省略しています。ソースコードを読んで不明点が合った場合は、これらを当たるようにしてみて下さい。

=== 理論概観

SSRの基本的な考え方は、レイトレーシングのテクニックを使い、カメラ、反射表面、オブジェクト（光源）、の関係をシミュレートするものです。

SSRでは通常の光学と異なり、カメラに入射する光の道筋から逆算することで、光源を特定した後、反射面にその色をフェッチしてくることで、反射面に反射が再現されます。

//image[komietty/ssr_idea][現実の光学とSSRの光の考え方の違い]{
//}

SSRではカメラの各ピクセルに対して、これを行います。

処理の大筋は以下のようにまとめることができます。

 1. スクリーン座標系を、深度情報を用いることでワールド座標系に戻す
 2. 視線ベクトルと法線情報から、反射ベクトルを求める
 3. 反射ベクトルを少しだけ伸ばし、その先端（＝レイ）の位置を再度スクリーン座標系に戻す
 4. スクリーン座標系のレイの位置について、レイの深度とデプスバッファに書き込まれている深度を比較する
 5. レイの方が深度が小さい場合、レイはまだ空中をさまよっている状態。３に戻りレイをもう少し進める
 6. レイの方が深度が大きい場合、レイがなんらか物体を通過したということなので、反射される色を取得できる
 7. もとのピクセルに戻り、取得した色を反映する

図では説明しにくい手続きですが、言葉で説明してもややこしいですね。分解して見ていきましょう。

=== 座標変換

先ずはスクリーン座標系とワールド座標系とを変換するための行列をシェーダーに渡してあげます。
@<code>{_ViewProj} がワールド座標系からスクリーン座標系への変換行列、@<code>{_InvViewProj} はその逆行列になります。

//emlist[SSR.cs]{
void OnRenderImage(RenderTexture src, RenderTexture dst)
{

  ....

  // world <-> screen matrix
  var view = cam.worldToCameraMatrix;
  var proj = GL.GetGPUProjectionMatrix(cam.projectionMatrix, false);
  var viewProj = proj * view;
  mat.SetMatrix("_ViewProj", viewProj);
  mat.SetMatrix("_InvViewProj", viewProj.inverse);

  ....

}
//}

さて渡された変換行列を用いて、法線ベクトルと反射ベクトルが求まります。該当するシェーダーの処理を見てみましょう。

//emlist[SSR.shader]{
float4 reflection (v2f i) : SV_Target
{
  float2 uv = i.screen.xy / i.screen.w;
  float depth = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uv);

  ...

  float2 screenpos = 2.0 * uv - 1.0;
  float4 pos = mul(_InvViewProj, float4(screenpos, depth, 1.0));
  pos /= pos.w;
  float3 camDir = normalize(pos - _WorldSpaceCameraPos);
  float3 normal = tex2D(_CameraGBufferTexture2, uv) * 2.0 - 1.0;
  float3 refDir = reflect(camDir, normal);

  ....

  if (_ViewMode == 1) col = float4((normal.xyz * 0.5 + 0.5), 1);
  if (_ViewMode == 2) col = float4((refDir.xyz * 0.5 + 0.5), 1);

  ....

  return col;
  }
//}

まず該当ピクセルの深度は @<code>{_CameraDepthTexture} に書き込まれており、これ利用します。
つぎにスクリーン上の位置情報と震度情報から、該当ピクセルに写っているポリゴンのワールド座標系での位置が分かるので、これを @<code>{pos} に保持します。
@<code>{pos} と @<code>{_WorldSpaceCameraPos} から、カメラへ向かうベクトルが分かるので、これと法線情報から、反射ベクトルが分かるようになります。

メインカメラにアタッチされたスクリプトから、法線ベクトルと反射ベクトルがどこを向いているかを確認できるようになってます。
各ベクトルとも -1 ~ 1 の間に規格化されているため、0以下の値の色情報は表示されません。ベクトルがｘ軸成分が大きいときは赤っぽく、ｙ軸成分が大きいときは緑っぽく、ｚ軸成分が大きいときは青っぽく表示されます。
ViewMode を @<code>{Normal} もしくは　@<code>{Reflection} にして確認してみて下さい。


=== レイトレーシング

では次にレイトレーシングを行う処理を見ていきましょう。

//emlist[SSR.shader]{
float4 reflection(v2f i) : SV_Target
{

  ...

  [loop]
  for (int n = 1; n <= _MaxLoop; n++)
  {
    float3 step = refDir * _RayLenCoeff * (lod + 1);
    ray += step * (1 + rand(uv + _Time.x) * (1 - smooth));

    float4 rayScreen  = mul(_ViewProj, float4(ray, 1.0));
    float2 rayUV      = rayScreen.xy / rayScreen.w * 0.5 + 0.5;
    float  rayDepth   = ComputeDepth(rayScreen);
    float  worldDepth = (lod == 0)?
           SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, rayUV) :
           tex2Dlod(_CameraDepthMipmap, float4(rayUV, 0, lod))
           + _BaseRaise * lod;

    ...

    if(rayDepth < worldDepth)
    {

      ....

      return outcol;
    }
  }
}
//}

後に説明する処理に関係する変数も混じっていますが、気にせず読み進めて下さい。
ループ内では、まずレイをステップ分だけ伸ばしてあげ、それを再度スクリーン座標系に戻します。スクリーン座標系でのレイの深度と、デプスバッファに書き込まれた深度を比較して、レイの方が奥にある場合、その色を返すことにします。
（デプスは最も近い時1.0で、遠くなるほど小さくなるので、@<code>{rayDepth} が @<code>{worldDepth} よりも小さい時、レイが奥にあるという判定になります。）

またループ回数が未定の場合はHLSLはエラーを吐くので、スクリプトからループの回数を渡したい場合は @<code>{[loop]} アトリビュートを先頭に書いておく必要があります。

レイトレーシングの骨組みの部分は以上で完成です。基本的な処理はイメージさえできてしまえばそんなに難しくありません。
ただし、きれいな反射を再現するためには、これからいくつか処理を追加していく必要があります。重要な改善すべきポイントとしては以下の４つが挙げられるでしょうか。

 1. ループ数に制限があるため、レイが進める距離が十分大きくならなず、遠くの物体の映り込みが再現できない
 2. レイのステップ数が大きくなると、映り込むオブジェクトを通過してしまい、間違った色をサンプリングしてしまう
 3. ループを多数回行うので、単純に処理が重い
 4. マテリアルの差異を考慮できていない

アンチエイリアスを含むポストエフェクトでは、処理の効率化のためのテクニックがむしろ本質的です。処理の骨子の理解が済んだところで、SSRを映像として成立させるテクニックを見ていきましょう。

=== Mipmap

以下では Chalmers University of Technologyの記事 @<fn>{chalmers} を参考に、Mipmap使うことで処理効率をあげる方法について解説します。(Mipmapが何かについては脚注参照 @<fn>{mipmap})
レイトレーシングはレイのステップ幅を決めてレイを徐々に進めていくのが基本ですが、Mipmapを使うことでオブジェクトとの交差判定までのレイのステップ幅を可変にすることができます。
こうすることで、限られたループ回数の中でも遠くまでレイを飛ばすことができるようになり、また処理効率も上がります。

RenderTextureからMipmapを使うデモシーンを用意してますのでそちらから確認していきましょう。


//emlist[Mipmap.cs]{
public class Mipmap : MonoBehaviour
{
  Material mat;
  RenderTexture rt;
  [SerializeField] Shader shader;
  [SerializeField] int lod;

  void OnEnable()
  {
    mat = new Material(shader);
    rt = new RenderTexture(Screen.width, Screen.height, 24);
    rt.useMipMap = true;
  }

  void OnDisable()
  {
    Destroy(mat);
    rt.Release();
  }

  void OnRenderImage(RenderTexture src, RenderTexture dst)
  {
    mat.SetInt("_LOD", lod);
    Graphics.Blit(src, rt);
    Graphics.Blit(rt, dst, mat);
  }
}
//}

既製のRenderTextureに対してはmipmapは設定できないようになっているので、ここでは新しくRenderTextureを生成し @<code>{src} をコピーしたあと、処理を加えています。

//emlist[Mipmap.shader]{
sampler2D _MainTex;
float4 _MainTex_ST;
int _LOD;

....

fixed4 frag (v2f i) : SV_Target
{
	return tex2Dlod(_MainTex, float4(i.uv, 0, _LOD));
}
//}

@<code>{tex2Dlod(_MainTex, float4(i.uv, 0, _LOD))}でLODに応じたMipmapが取得できるようになってます。

シーン上でカメラにアタッチされたスクリプトからLODを上げていくと、画像が粗くなっていくのが確認できるかと思います。

//image[komietty/ssr_mipmap_demo][LODの上昇とMipmapの画質の比較]{
//}

Mipmapの使い方が確認できたところで、SSRシーンのなかでMipmapがどのように利用されているか見ていきましょう。

//emlist[SSR.shader]{
[loop]
for (int n = 1; n <= _MaxLoop; n++)
{
  float3 step = refDir * _RayLenCoeff * (lod + 1);
  ray += step;

  ....

  if(rayDepth < worldDepth)
  {
    if(lod == 0)
    {
      if (rayDepth + _Thickness > worldDepth)
      {
        float sign = -1.0;
        for (int m = 1; m <= 8; ++m)
        {
          ray += sign * pow(0.5, m) * step;
          rayScreen = mul(_ViewProj, float4(ray, 1.0));
          rayUV = rayScreen.xy / rayScreen.w * 0.5 + 0.5;
          rayDepth = ComputeDepth(rayScreen);
          worldDepth = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, rayUV);
          sign = (rayDepth < worldDepth) ? -1 : 1;
        }
        refcol = tex2D(_MainTex, rayUV);
      }
      break;
    }
    else
    {
      ray -= step;
      lod--;
    }
  }
  else if(n <= _MaxLOD)
  {
    lod++;
  }
  calcTimes = n;
}
if (_ViewMode == 3) return float4(1, 1, 1, 1) * calc / _MaxLoop;

....

//}

Chalmersの記事にある図を用いて解説を進めていきます。

//image[komietty/ssr_mipmap_logic][Mipmapを用いた計算方法]{
//}

図のように、最初の数回は慎重に交差判定を行いながらLODを上げてきます。他メッシュとの交差が無いうちは、大きなステップのまま進めていきます。
交差があった場合、UnityのMipMapは平均値を取りながら画素を荒くしていくので、記事の場合とは異なり、レイが行き過ぎてしまう場合があります。そのため一旦単位ステップ分後退し、
再度一つ小さなLODでレイを進めます。最終的にLOD=0で画像で交差判定を行うことで、レイの移動距離を伸ばし、処理を効率化することができます。

メインカメラにアタッチされたスクリプトから、LODを上げた場合どのくらい計算量が変化するかが確認できるようになっています。計算量が多いほど白っぽく、少ないほど黒っぽく見えるようにしています。
ViewMode を @<code>{CalcCount} にしてLODを変更しながら計算量の変化を確認してみて下さい。

//image[komietty/ssr_lod][LODの変化による計算量の違い（黒に近いほど計算量が少ない）]{
//}

=== 二分木探索

二分木探索で交差近傍の精度を上げていく方法を見ていきましょう。
早速コードから確認します。

//emlist[SSR.shader]{
if (lod == 0)
{
  if (rayDepth + _Thickness > worldDepth)
  {
    float sign = -1.0;
    for (int m = 1; m <= 8; ++m)
    {
      ray += sign * pow(0.5, m) * step;
      rayScreen = mul(_ViewProj, float4(ray, 1.0));
      rayUV = rayScreen.xy / rayScreen.w * 0.5 + 0.5;
      rayDepth = ComputeDepth(rayScreen);
      worldDepth = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, rayUV);
      sign = (rayDepth < worldDepth) ? -1 : 1;
    }
    refcol = tex2D(_MainTex, rayUV);
  }
  break;
}
//}

交差の直後は交差したオブジェクトよりも奥にあるので、まずレイを後退させます。その後レイとメッシュとの前後関係を確認しながら、レイの進行方向を前後どちらか変更していきます。
同時にレイのステップ幅を短くすることで、より少ない誤差でメッシュとの交差位置を特定することができます。


=== マテリアルの違いを反映

ここまでの方法ではスクリーン内のオブジェクトのマテリアルの違いは考慮していませんでした。そのため、全てのオブジェクトが同程度に反射してしまうという問題があります。
そこで、再度G-bufferを使います。 @<code>{_CameraGBufferTexture1.w} にはマテリアルのsmoothnessが格納されているので、これを使います。

//emlist[SSR.shader]{
if (_ViewMode == 8)
  return float4(1, 1, 1, 1) * tex2D(_CameraGBufferTexture1, uv).w;

....

return
  (col * (1 - smooth) + refcol * smooth) * _ReflectionRate
　+ col * (1 - _ReflectionRate);
//}

シーン内のオブジェクトに付帯しているマテリアルのsmoothnessの値を変更すると、そのオブジェクトだけ反射の程度が変更しているのが見て取れます。
またメインカメラにアタッチされたスクリプトのViewMode を @<code>{Smoothness} にすることで、シーン内のsmoothnessを一覧できます。白っぽいほどsmoothnessが大きくなっています。


=== Blur処理

第一節で解説したガウシアンブラー使用している部分です。
レイのステップ幅が十分小さくない場合、二分木探索を行っても反射をうまく取得できないことがあります。
レイのステップ幅を小さくするとレイの全長が短くなってしまい、また計算量も増えるので、ステップ幅はただ小さくすれば良いというものではなく、適当な小ささにとどめておく必要があります。
反射を上手く取得できなかった部分はブラー処理を掛けて、それらしく見せていきます。

//emlist[SSR.shader]{
float4 xblur(v2f i) : SV_Target
{
  float2 uv = i.screen.xy / i.screen.w;
  float2 size = _ReflectionTexture_TexelSize;
  float smooth = tex2D(_CameraGBufferTexture1, uv).w;

  // compare depth
  float depth = SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uv);
  float depthR =
        SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uv + float2(1, 0) * size);
  float depthL =
        SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uv - float2(1, 0) * size);

  if (depth <= 0) return tex2D(_ReflectionTexture, uv);

  float weight[5] = { 0.2270270, 0.1945945, 0.1216216, 0.0540540, 0.0162162 };
  float offset[5] = { 0.0, 1.0, 2.0, 3.0, 4.0 };

  float4 originalColor = tex2D(_ReflectionTexture, uv);
  float4 blurredColor = tex2D(_ReflectionTexture, uv) * weight[0];

  for (int j = 1; j < 5; ++j)
  {
    blurredColor
        += tex2D(_ReflectionTexture, uv + float2(offset[j], 0) * size)
           * weight[j];

    blurredColor
        += tex2D(_ReflectionTexture, uv - float2(offset[j], 0) * size)
           * weight[j];
  }

  float4 o = (abs(depthR - depthL) > _BlurThreshold) ? originalColor
  　　　　　　　: blurredColor * smooth + originalColor * (1 - smooth);
  return o;
}
//}

ここでも前述の理由から @<code>{xblur} と @<code>{yblur} で処理を分けています。
またブラー処理を掛けたいのは同一の反射面内のみなので、輪郭部分にはブラー処理が行われないようにしています。
左右のデプスの差分が大きい場合、輪郭部分と判定しています。（ @<code>{yblur} では上下の差分を評価しています。）

ここまでの処理を追加した結果が以下になります。

//image[komietty/ssr_result][結果]{
//}

=== おまけ

おまけ程度ですが、メインカメラとサブカメラの２台を使って、存在しないオブジェクトが写り込んでいるかのようなテクニックを紹介します。

//emlist[SSRMainCamera.shader]{
float4 reflection(v2f i) : SV_Target
{

  ....

  for (int n = 1; n <= 100; ++n)
  {
    float3 ray = n * step;
    float3 rayPos = pos + ray;
    float4 vpPos = mul(_ViewProj, float4(rayPos, 1.0));
    float2 rayUv = vpPos.xy / vpPos.w * 0.5 + 0.5;
    float rayDepth = vpPos.z / vpPos.w;
    float subCameraDepth = SAMPLE_DEPTH_TEXTURE(_SubCameraDepthTex, rayUv);

    if (rayDepth < subCameraDepth && rayDepth + thickness > subCameraDepth)
    {
      float sign = -1.0;
      for (int m = 1; m <= 4; ++m)
      {
        rayPos += sign * pow(0.5, m) * step;
        vpPos = mul(_ViewProj, float4(rayPos, 1.0));
        rayUv = vpPos.xy / vpPos.w * 0.5 + 0.5;
        rayDepth = vpPos.z / vpPos.w;
        subCameraDepth = SAMPLE_DEPTH_TEXTURE(_SubCameraDepthTex, rayUv);
        sign = rayDepth - subCameraDepth < 0 ? -1 : 1;
      }
      col = tex2D(_SubCameraMainTex, rayUv);
    }
  }
  return col * smooth + tex2D(_MainTex, uv) * (1 - smooth);
}
//}

極力余分な処理を省いたシンプルな作りにしています。ポイントはデプス評価のために @<code>{SAMPLE_DEPTH_TEXTURE(_CameraDepthTexture, uv)} の代わりに @<code>{SAMPLE_DEPTH_TEXTURE(_SubCameraDepthTex, rayUv)} を使い、参照するオブジェクト情報も @<code>{_SubCameraMainTex} から取得している点です。
@<code>{_CameraDepthTexture}, @<code>{_SubCameraDepthTex} はサブカメラからグローバルテクチャとしてセットしています。

欠点は、お互いのカメラが影になって映るべきでないオブジェクトも写してしまう点です。実用性はそれほどないかもしれませんが、ちょっとしたおもしろエフェクトということで。

//image[komietty/ssr_twocam][カメラ２台を用いた方法]{
//}

== まとめ

以上でSSRの解説は終わりです。

SSRは大きな処理容量が必要になってくる手法のため、全ての位置にあるオブジェクトをきれいに反射させることは現実的ではありません。
そこで、注目するオブジェクトの反射の見映えを良くしながら、些末な反射をいかに少ない処理でそれらしく見せるかがポイントになってきます。
またレンダリングされるスクリーンサイズがそのまま計算量に直結するので、想定されるスクリーンサイズと、GPUの性能を加味しながら、映像として成立するポイントを探っていくことが重要です。
シーン内のオブジェクトを動かしながら、パラメータを調整することで、各パラメータの役割とトレードオフを確認してみて下さい。

また、ここまでに挙げた、Mipmapや二分木探索、カメラバッファの使い方、その他数々の細かいテクニックはSSRに限らず様々なところで応用が利きます。
読者の方々にとって部分的にでも参考になる内容があれば幸いです。


//footnote[gaussian][http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/]
//footnote[binomial][https://ja.wikipedia.org/wiki/%E4%BA%8C%E9%A0%85%E5%88%86%E5%B8%83]
//footnote[shirley][https://www.amazon.co.jp/gp/product/B01B5AODD8]
//footnote[texel][https://msdn.microsoft.com/ja-jp/library/bb219690(v=vs.85).aspx]
//footnote[reflection][https://www.sciencelearn.org.nz/resources/48-reflection-of-light]
//footnote[kode80][http://www.kode80.com/blog/2015/03/11/screen-space-reflections-in-unity-5/]
//footnote[chalmers][http://www.cse.chalmers.se/edu/year/2017/course/TDA361/　　　　　　Advanced%20Computer%20Graphics/Screen-space%20reflections.pdf]
//footnote[hecomi][http://tips.hecomi.com/entry/2016/04/04/022550]
//footnote[mipmap][https://answers.unity.com/questions/441984/what-is-mip-maps-pictures.html]
//footnote[gbuffer][https://docs.unity3d.com/Manual/RenderTech-DeferredShading.html]
