= Projection Spray

== はじめに

こんにちは！すぎのひろのりです！前回の『UnityGraphicsPrograming Vol1』ではキチンと記事書けなくてすみませんでした！代筆してくれた大石くんには、感謝です(._.)

ここでは、前回書けなかったスプレーのプログラムについて、解説していきたいと思います。Unityのコード的には、Vol1時点のモノよりちょっと良くなりました！

まず、UnityのBuilt-inのライトを使用せずにシンプルな照明効果を独自実装してみます。そして、その応用として、3Dオブジェクトをスプレーでペイントする処理の開発について、解説していきます。
この章のコンセプトとしては、UnityCG.cgincやビルトインの処理を参考に機能を自作し、新しい機能に応用していくという流れを追えたらな。と思います。

=== サンプルリポジトリ

本章のサンプルは、@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming2} のProjectionSprayフォルダ内です。

== LightComponentの実装

UnityのLightはものすごく便利です。ライトオブジェクトを設置するだけで、世界は明るくなります。Inspectorから影を選択すると、自動的にシャドウマップが作られ、オブジェクトから影が落ちます。

まずは、Unityがどのようにライトを実装しているのかを見ながら、独自にライトを実装していきます。

=== Unity Built-in Shader

Unityでは、デフォルトで入っているマテリアルのシェーダーや、内部的に使われているCGINCファイルをUnityダウンロードアーカイブから、ダウンロードすることができます。

独自のシェーダーを書くときの参考になったり、より深くCGの描写について知ることができるので、是非、ダウンロードして見ることをお勧めします！

//image[downloadBuilt-inShader][https://unity3d.com/jp/get-unity/download/archive]{
//}

本章で関係ありそうな、ライティング関連のファイルは以下です

 * CGIncludes/UnityCG.cginc
 * CGIncludes/AutoLight.cginc
 * CGIncludes/Lighting.cginc
 * CGIncludes/UnityLightingCommon.cginc

基本的なLambertLightingについて見てみます。（@<list>{LightingLambert}）ランバートさんが考えました。

//listnum[LightingLambert][Lighting.cginc]{
struct SurfaceOutput {
    fixed3 Albedo;
    fixed3 Normal;
    fixed3 Emission;
    half Specular;
    fixed Gloss;
    fixed Alpha;
};
~~
inline fixed4 UnityLambertLight (SurfaceOutput s, UnityLight light)
{
    fixed diff = max (0, dot (s.Normal, light.dir));

    fixed4 c;
    c.rgb = s.Albedo * light.color * diff;
    c.a = s.Alpha;
    return c;
}
//}

実際のライティング計算ですが、ライトからメッシュまでの方向とメッシュの法線方向との内積から、ディフューズの値を計算しています。@<list>{LightingLambert}

@<code>{fixed diff = max (0, dot (s.Normal, light.dir));}

Lighting.cginc内で未定義のUnityLightについては、UnityLightingCommon.cginc内で定義していて、ライトの色と方向の情報が格納されています。@<list>{UnityLight}

//listnum[UnityLight][UnityLightingCommon.cginc]{
struct UnityLight
{
    half3 color;
    half3 dir;

    // Deprecated: Ndotl is now calculated on the fly 
    // and is no longer stored. Do not used it.
    half  ndotl; 
};
//}

=== メッシュの法線の表示

実際のライティングの処理を見てみて、ライティングの計算にはメッシュの法線（Normal）情報が必要だということが分かったので、Shaderでメッシュの法線情報を表示する方法を軽く見ていきます。

サンプルプロジェクト内の@<b>{00_viewNormal.unity}のシーンを参照してください。

オブジェクトに、法線情報をカラーとして出力するマテリアルが設定してあり、そのシェーダーは@<list>{showNormal}のようになっています。

//listnum[showNormal][simple-showNormal.shader]{
struct appdata
{
    float4 vertex : POSITION;
    float3 normal : NORMAL;
};

struct v2f
{
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 vertex : SV_POSITION;
};

v2f vert (appdata v)
{
    v2f o;
    o.vertex = UnityObjectToClipPos(v.vertex);
    o.normal = UnityObjectToWorldNormal(v.normal);
    return o;
}

half4 frag (v2f i) : SV_Target
{
    fixed4 col = half4(i.normal,1);
    return col;
}
//}

頂点シェーダ（v2f vert）で、メッシュのワールド座標系における法線方向を算出しフラグメントシェーダ（half4 frag）に渡します。
フラグメントシェーダでは渡された法線情報を、x成分をR、y成分をG, z成分をBとして、カラーに変換してそのまま出力しています。@<list>{showNormal}

イメージ上は黒に見える部分でも、実際は法線ｘに負の値が入っている場合もあります。@<img>{buddha_normal}

//image[buddha_normal][00_viewNormal.unity]{
//}

これで、メッシュにライティングを施す準備ができました。

===[column] ビルトインシェーダーヘルパー機能

UnityCG.cginc内にはシェーダーを簡単に書くためのビルトインユーティリティ関数があります。
たとえば、@<list>{showNormal}で使われている@<code>{UnityObjectToClipPos}は頂点の位置をオブジェクト（ローカル）座標系からクリップ座標系に変換します。
また、@<code>{UnityObjectToWorldNormal}の関数は法線方向をオブジェクト座標系からワールド座標系に変換しています。

シェーダーの記述に便利なので、その他の関数に関しては、UnityCG.cgincを参照するか、公式マニュアルをご参照下さい。
@<href>{https://docs.unity3d.com/ja/current/Manual/SL-BuiltinFunctions.html}

また、座標変換や各座標系について詳しく知りたい場合は、Unity Graphics Programming vol.1、福永さんの『第9章 Multi Plane Perspective Projection』をご参照いただけると、詳しくなれるかもしれません。

===[/column]

=== 点光源（PointLight）

サンプルプロジェクト内の@<b>{01_pointLight.unity}のシーンを参照してください。

//image[scene-pointLight][01_pointLight.unity]{
//}

点光源は、ある一点から全方向を照らす光源です。シーンには、BuddhaのメッシュオブジェクトとPointLightのオブジェクトがあります。
PointLightオブジェクトには、メッシュにライトの情報を送るためのスクリプト（@<list>{pointLight-cs}）が付いていて、
そのライトの情報をもとに、ライティングした結果をマテリアル（@<list>{pointLight-shader}）で表示しています。

//listnum[pointLight-cs][PointLightComponent.cs]{
using UnityEngine;

[ExecuteInEditMode]
public class PointLightComponent : MonoBehaviour
{
    static MaterialPropertyBlock mpb;

    public Renderer targetRenderer;
    public float intensity = 1f;
    public Color color = Color.white;

    void Update()
    {
        if (targetRenderer == null)
            return;
        if (mpb == null)
            mpb = new MaterialPropertyBlock();

        targetRenderer.GetPropertyBlock(mpb);
        mpb.SetVector("_LitPos", transform.position);
        mpb.SetFloat("_Intensity", intensity);
        mpb.SetColor("_LitCol", color);
        targetRenderer.SetPropertyBlock(mpb);
    }

    private void OnDrawGizmos()
    {
        Gizmos.color = color;
        Gizmos.DrawWireSphere(transform.position, intensity);
    }
}
//}

このコンポーネントは、ライトの位置と強さと色を対象のメッシュに受け渡しています。
そして、受け取った情報をもとにライティング処理をするマテリアルが設定してあります。

マテリアルの @<code>{"_LitPos"}, @<code>{"_LitCol"}, @<code>{"_Intensity"}の各プロパティに、CSharpから値が設定されています。

//listnum[pointLight-shader][simple-pointLight.shader]{
Shader "Unlit/Simple/PointLight-Reciever"
{
    Properties
    {
        _LitPos("light position", Vector) = (0,0,0,0)
        _LitCol("light color", Color) = (1,1,1,1)
        _Intensity("light intensity", Float) = 1
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float3 normal : NORMAL;
            };

            struct v2f
            {
                float3 worldPos : TEXCOORD0;
                float3 normal : TEXCOORD1;
                float4 vertex : SV_POSITION;
            };

            half4 _LitPos, _LitCol;
            half _Intensity;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.worldPos = mul(unity_ObjectToWorld, v.vertex).xyz;
                //ワールド座標系におけるメッシュの位置をフラグメントシェーダに渡す
                o.normal = UnityObjectToWorldNormal(v.normal);
                return o;
            }

            fixed4 frag (v2f i) : SV_Target
            {
                half3 to = i.worldPos - _LitPos;
                //ライトの位置からメッシュ位置へのベクトル
                half3 lightDir = normalize(to);
                half dist = length(to);
                half atten = 
                    _Intensity * dot(-lightDir, i.normal) / (dist * dist);

                half4 col = max(0.0, atten) * _LitCol;
                return col;
            }
            ENDCG
        }
    }
}
//}

ライティングの計算は、基本のLambertLighting（@<list>{LightingLambert}）の計算をもとに拡散光を計算し、強さは距離の二乗に反比例して減衰するようにしている。@<list>{pointLight-shader}

@<code>{half atten = _Intensity * dot(-lightDir, i.normal) / (dist * dist);}

１つのモデルに対して１つのライトの簡単なシステムですが、ライティング処理を実装できました。

//image[buddha_pointLight][01_pointLight.unity]{
//}

=== スポットライト（SpotLight）

次に、スポットライトの実装をしてみます。スポットライトは、ポイントライトと違い、方向を持ったライトで、一方方向に向けて光を出しています。

//image[scene-spotLight][02_spotLight.unity]{
//}

ここでは、スポットライトのGizmo表示のためだけにUnity標準のライトを使用しています。@<img>{scene-spotLight}

スポットライトは方向性があるため、位置情報の他にライトの向き、スポットアングル（角度）の情報がポイントライトのときに加えて増えます。
これらの情報はライトの@<code>{worldToLightMatrix}、@<code>{projectionMatrix}としてそれぞれ@<code>{Matrix4x4}（Shaderだと@<code>{float4x4}）のプロパティで受け渡します。

さらに、スポットライトはLightCookieを設定することもできます。（UnityにはデフォルトのLightCookieがあるのですが、エディタから選択できなかったので、Default-Particleのテクスチャを使用しています）

//listnum[spotLight-csharp][SpotLightComponent.cs]{
using UnityEngine;

[ExecuteInEditMode]
public class SpotLightComponent : MonoBehaviour
{
    static MaterialPropertyBlock mpb;

    public Renderer targetRenderer;
    public float intensity = 1f;
    public Color color = Color.white;
    [Range(0.01f, 90f)] public float angle = 30f;
    public float range = 10f;
    public Texture cookie;

    void Update()
    {
        if (targetRenderer == null)
            return;
        if (mpb == null)
            mpb = new MaterialPropertyBlock();

        //projectionMatrixを計算している
        var projMatrix = Matrix4x4.Perspective(angle, 1f, 0f, range);
        var worldToLightMatrix = transform.worldToLocalMatrix;

        targetRenderer.GetPropertyBlock(mpb);
        mpb.SetVector("_LitPos", transform.position);
        mpb.SetFloat("_Intensity", intensity);
        mpb.SetColor("_LitCol", color);
        mpb.SetMatrix("_WorldToLitMatrix", worldToLightMatrix);
        mpb.SetMatrix("_ProjMatrix", projMatrix);
        mpb.SetTexture("_Cookie", cookie);
        targetRenderer.SetPropertyBlock(mpb);
    }
}    
//}

projectionMatrixは、@<code>{Matrix4x4.Perspective(angle, 1f, 0f, range)}で、算出しています。

スポットライトから受け取ったパラメータ情報をもとにShaderでライティング処理を計算し、表示します。@<list>{spotLight-shader}

//listnum[spotLight-shader][simple-spotLight.shader]{
uniform float4x4 _ProjMatrix, _WorldToLitMatrix;

sampler2D _Cookie;
half4 _LitPos, _LitCol;
half _Intensity;

~~

fixed4 frag (v2f i) : SV_Target
{
    half3 to = i.worldPos - _LitPos.xyz;
    half3 lightDir = normalize(to);
    half dist = length(to);
    half atten = _Intensity * dot(-lightDir, i.normal) / (dist * dist);

    half4 lightSpacePos = mul(_WorldToLitMatrix, half4(i.worldPos, 1.0));
    half4 projPos = mul(_ProjMatrix, lightSpacePos);
    projPos.z *= -1;
    half2 litUv = projPos.xy / projPos.z;
    litUv = litUv * 0.5 + 0.5;
    half lightCookie = tex2D(_Cookie, litUv);
    lightCookie *= 
        0<litUv.x && litUv.x<1 && 0<litUv.y && litUv.y<1 && 0<projPos.z;

    half4 col = max(0.0, atten) * _LitCol * lightCookie;
    return col;
}

//}

基本的に、フラグメントシェーダー以外はポイントライトと同じだということが分かるかと思います。@<list>{spotLight-shader}

16行目から22行目にかけてが、スポットライトの各地点における強度を計算しています。ライトの位置から見たとき、その地点がライトの範囲に入っているかと、その地点におけるライトCookieをサンプリングしてライトの強度を求めます。

//image[buddha_spotLight][02_spotLight.unity]{
//}

===[column] ビルトインのCookieの処理

スポットライトのCookieの処理については、ビルトインCGINC、AutoLight.cginc内の@<code>{UnitySpotCookie()}の部分が参考になります。

//listnum[UnitySpotCookie][AutoLight.cginc]{
#ifdef SPOT
sampler2D _LightTexture0;
unityShadowCoord4x4 unity_WorldToLight;
sampler2D _LightTextureB0;
inline fixed UnitySpotCookie(unityShadowCoord4 LightCoord)
{
    return tex2D(
        _LightTexture0, 
        LightCoord.xy / LightCoord.w + 0.5
    ).w;
}
inline fixed UnitySpotAttenuate(unityShadowCoord3 LightCoord)
{
    return tex2D(
        _LightTextureB0, 
        dot(LightCoord, LightCoord).xx
    ).UNITY_ATTEN_CHANNEL;
}
#define UNITY_LIGHT_ATTENUATION(destName, input, worldPos) \
    unityShadowCoord4 lightCoord = mul( \
        unity_WorldToLight, \
        unityShadowCoord4(worldPos, 1) \
    ); \
    fixed shadow = UNITY_SHADOW_ATTENUATION(input, worldPos); \
    fixed destName = \
        (lightCoord.z > 0) * \
        UnitySpotCookie(lightCoord) * \
        UnitySpotAttenuate(lightCoord.xyz) * shadow;
#endif
//}

===[/column]

=== Shadow 影の実装

ライティングの実装として最後に、影を実装してみます。

ライトから光が出て、直接光が当たったメッシュは明るくなり、ライトとメッシュの間に別の何かがあり、光が遮られたメッシュは暗くなります。これが影です。

手順としては、ざっくりと

 * ライトから見た深度テクスチャを作成する。
 * オブジェクトレンダリング時、その地点のライトからの深度が深度テクスチャよりも大きかったらその地点は他のオブジェクトに遮られているので影になる。

という形になります。今回はライトの位置からの深度テクスチャが必要になるので、SpotLightにCameraコンポーネントを付けて、ライトから見た深度テクスチャを作成します。

//image[scene-spotShadow][03_spotLight-withShadow.unity]{
//}

SpotLightComponent（自作）にCamera（ビルトイン）が付いています。@<img>{scene-spotShadow}

//listnum[spotShadow-csharp][SpotLightWithShadow.cs]{
Shader depthRenderShader { 
    get { return Shader.Find("Unlit/depthRender"); } 
}

new Camera camera
{
    get
    {
        if (_c == null)
        {
            _c = GetComponent<Camera>();
            if (_c == null)
                _c = gameObject.AddComponent<Camera>();
            depthOutput = new RenderTexture(
                shadowMapResolution, 
                shadowMapResolution, 
                16, 
                RenderTextureFormat.RFloat
            );
            depthOutput.wrapMode = TextureWrapMode.Clamp;
            depthOutput.Create();
            _c.targetTexture = depthOutput;
            _c.SetReplacementShader(depthRenderShader, "RenderType");
            _c.clearFlags = CameraClearFlags.Nothing;
            _c.nearClipPlane = 0.01f;
            _c.enabled = false;
        }
        return _c;
    }
}
Camera _c;
RenderTexture depthOutput;

void Update()
{
    if (mpb == null)
        mpb = new MaterialPropertyBlock();

    var currentRt = RenderTexture.active;
    RenderTexture.active = depthOutput;
    GL.Clear(true, true, Color.white * camera.farClipPlane);
    camera.fieldOfView = angle;
    camera.nearClipPlane = 0.01f;
    camera.farClipPlane = range;
    camera.Render();
    //カメラのレンダリングはスクリプト上、マニュアルで行う
    RenderTexture.active = currentRt;

    var projMatrix = camera.projectionMatrix;
    //プロジェクション行列は、カメラのものを使う
    var worldToLightMatrix = transform.worldToLocalMatrix;

    ~~
}
//}

C#スクリプトはほとんど影なしバージョンと同じなのですが、深度テクスチャをレンダリングするカメラと深度をレンダリングするReplacementShaderの設定をしています。
また、今回はカメラがあるので、プロジェクション行列は、@<code>{Matrix4x4.Perspective}ではなく、@<code>{Camera.projectionMatrix}を使用します。

深度テクスチャ生成用シェーダは、次のコードになります。

//listnum[depthRender][depthRender.shader]{
    v2f vert (float4 pos : POSITION)
    {
        v2f o;
        o.vertex = UnityObjectToClipPos(pos);
        o.depth = abs(UnityObjectToViewPos(pos).z);
        return o;
    }

    float frag (v2f i) : SV_Target
    {
        return i.depth;
    }
//}

生成した深度テクスチャ（@<img>{depth}）ライト座標系（カメラ座標系）におけるオブジェクトの位置のz座標の値が出力されています。

//image[depth][light depthTexture]{
//}

生成した深度テクスチャ（@<code>{depthOutput}）をメッシュオブジェクトに渡し、オブジェクトをレンダリングします。
オブジェクトの影の計算部分は、次のようになります。

//listnum[spotShadow-shader][simple-spotLight-withShadow.shader]{
fixed4 frag (v2f i) : SV_Target
{
    ///diffuse lighting
    half3 to = i.worldPos - _LitPos.xyz;
    half3 lightDir = normalize(to);
    half dist = length(to);
    half atten = _Intensity * dot(-lightDir, i.normal) / (dist * dist);

    ///spot-light cookie
    half4 lightSpacePos = mul(_WorldToLitMatrix, half4(i.worldPos, 1.0));
    half4 projPos = mul(_ProjMatrix, lightSpacePos);
    projPos.z *= -1;
    half2 litUv = projPos.xy / projPos.z;
    litUv = litUv * 0.5 + 0.5;
    half lightCookie = tex2D(_Cookie, litUv);
    lightCookie *= 
        0<litUv.x && litUv.x<1 && 0<litUv.y && litUv.y<1 && 0<projPos.z;

    ///shadow
    half lightDepth = tex2D(_LitDepth, litUv).r;
    //_LitDepthにライトから見た深度テクスチャが渡されている
    atten *= 1.0 - saturate(10*abs(lightSpacePos.z) - 10*lightDepth);

    half4 col = max(0.0, atten) * _LitCol * lightCookie;
    return col;
}
//}

カメラによって作られた深度テクスチャ@<code>{tex2D(_LitTexture, litUv).r}と@<code>{lightSpacePos.z}は、どちらもライトから見たオブジェクトの頂点位置のz値が格納されています。
テクスチャである@<code>{_LitTexture}はライトから見えている面＝光が当たっている面の情報なので、深度テクスチャからサンプルした値（@<code>{lightDepth}）と@<code>{lightSpacePos.z}を比較して、影かどうかの判定をしている。

@<code>{atten *= 1.0 - saturate(10*abs(lightSpacePos.z) - 10*lightDepth);}

ここのコードで、@<code>{lightDepth}より@<code>{lightSpacePos.z}の値が大きかったら、サーフェイスは暗くなります。

//image[buddha_spotShadow][03_spotLight-withShadow.unity]{
//}

これで、スポットライトでオブジェクトの影を表示できました。

このスポットライトと影の実装を使って、オブジェクトにリアルタイムで色を塗る、スプレーの機能を実装していきます。

===[column] Camera.projectionMatrixとMatrix4x4.Perspectiveは同じ行列

Unityシーン：Example内、@<b>{compareMatrix.unity}

//listnum[matrix][CompareMatrix.cs][csharp]{
    float fov = 30f;
    float near = 0.01f;
    float far = 1000f;

    camera.fieldOfView = fov;
    camera.nearClipPlane = near;
    camera.farClipPlane = far;

    Matrix4x4 cameraMatrix = camera.projectionMatrix;
    Matrix4x4 perseMatrix = Matrix4x4.Perspective(
        fov, 
        1f, 
        near, 
        far
    );
//}

===[/column]

== ProjectionSprayの実装

ここからは、自作したSpotLightComponentを応用して、オブジェクトに色を塗れるスプレーの機能を実装していきます。

基本的には、ライティングの強度の値をもとに、オブジェクトのテクスチャに描き込みを行います。今回使用している、Buddhaのオブジェクトはuvデータが存在しないので、そのままではテクスチャを貼ることができないのですが、UnityにはLightMap用のUVを生成する機能があります。

//image[buddha-importSetting][buddha Import Setting]{
//}

モデルのImportSettingにおいて、"Generate Lightmap UVs"の項目にチェックを入れると、ライトマップ用のUVが生成されます。（@<code>{v.uv2 : TEXCOORD1}）
このUv2用に描き込み可能なRenderTextureを作成し、描き込みを行います。

=== showUv2

サンプルシーンは、@<b>{00_showUv2.unity}を参照してください。

mesh.uv2にマッピングするテクスチャに書き込むためには、メッシュからUV2に展開したテクスチャを生成する必要があります。まずは、メッシュの頂点をUV2の座標に展開するシェーダーを作ってみます。

//image[scene-showUv2][00_showUv2.unity]{
//}

シーン内でBuddhaオブジェクトを選択し、マテリアルの"slider"のパラメータを操作すると、オブジェクトが元の形状からUv2に展開された形状に変化します。色付けは、@<code>{uv2.xy}を@<code>{color.rg}に割り当てています。

//listnum[showUv2-shader][showUv2.shader]{
float _T;

v2f vert(appdata v)
{
#if UNITY_UV_STARTS_AT_TOP
    v.uv2.y = 1.0 - v.uv2.y;
#endif
    float4 pos0 = UnityObjectToClipPos(v.vertex);
    float4 pos1 = float4(v.uv2*2.0 - 1.0, 0.0, 1.0);

    v2f o;
    o.vertex = lerp(pos0, pos1, _T);
    o.uv2 = v.uv2;
    o.worldPos = mul(unity_ObjectToWorld, v.vertex).xyz;
    o.normal = UnityObjectToWorldNormal(v.normal);
    return o;
}

half4 frag(v2f i) : SV_Target
{
    return half4(i.uv2,0,1);
}
//}

@<code>{float4 pos1 = float4(v.uv2*2.0 - 1.0, 0.0, 1.0);} の値が、クリップ座標系におけるUv2に展開された位置になります。@<list>{showUv2-shader}

フラグメントシェーダに@<code>{worldPos}と@<code>{normal}の値を渡しているので、この値を使用してスポットライトの計算におけるライティングの処理を行います。

//image[buddha-uv2][00_showUv2.unity]{
//}

メッシュからUv2に展開したテクスチャを生成できる！

=== ProjectionSpray

それでは、準備が整ったので、スプレーの機能を実装します。@<b>{01_projectionSpray.unity}のシーンを参照してください。

//image[scene-spray][01_projectionSpray.unity]{
//}

このシーンを実行すると、黒いBuddhaのオブジェクトにだんだん、色が付いていきます。そして、画面をクリックすると、その部分にスプレーでカラフルな色が塗られます。

実装の内容的には、今まで実装してきた自作スポットライトの応用になります。
スポットライトのライティングの計算をそのままライティングに使用せず、@<code>{RenderTexture}の更新に使用しています。
この例では、書き込んだテクスチャをライトマップ用に生成された@<code>{mesh.uv2}にマッピングしています。

@<code>{Drawable}はスプレーで書き込まれる対象のオブジェクトに付いているコンポーネントで、テクスチャに描き込む処理を行なっています。
@<code>{ProjectionSpray}コンポーネントは、スプレーの位置などのテクスチャに描き込みを行う@<code>{Material}のプロパティの設定を行なっています。
処理の流れとしては、@<code>{DrawableController}の@<code>{Update}関数内で、@<code>{projectionSpray.Draw(drawable)}を呼び、テクスチャに描き込みを行なっています。

==== ProjectionSpray.cs

 * @<code>{Material drawMat}: 描き込みを行うためのマテリアル
 * @<code>{UpdateDrawingMat()}: 描き込み前にマテリアルの設定を更新する
 * @<code>{Draw(Drawable drawable)}: @<code>{drawMat}を@<code>{drawable.Draw(Material mat)}に渡して、描き込みを行う。

//listnum[spray-projectionSpray][projectionSpray.cs]{
public class ProjectionSpray : MonoBehaviour {

    public Material drawingMat;

    public float intensity = 1f;
    public Color color = Color.white;
    [Range(0.01f, 90f)] public float angle = 30f;
    public float range = 10f;
    public Texture cookie;
    public int shadowMapResolution = 1024;

    Shader depthRenderShader { 
        get { return Shader.Find("Unlit/depthRender"); } 
    }

    new Camera camera{get{~~}}
    Camera _c;
    RenderTexture depthOutput;

    public void UpdateDrawingMat()
    {
        var currentRt = RenderTexture.active;
        RenderTexture.active = depthOutput;
        GL.Clear(true, true, Color.white * camera.farClipPlane);
        camera.fieldOfView = angle;
        camera.nearClipPlane = 0.01f;
        camera.farClipPlane = range;
        camera.Render();
        //深度テクスチャの更新
        RenderTexture.active = currentRt;

        var projMatrix = camera.projectionMatrix;
        var worldToDrawerMatrix = transform.worldToLocalMatrix;

        drawingMat.SetVector("_DrawerPos", transform.position);
        drawingMat.SetFloat("_Emission", intensity * Time.smoothDeltaTime);
        drawingMat.SetColor("_Color", color);
        drawingMat.SetMatrix("_WorldToDrawerMatrix", worldToDrawerMatrix);
        drawingMat.SetMatrix("_ProjMatrix", projMatrix);
        drawingMat.SetTexture("_Cookie", cookie);
        drawingMat.SetTexture("_DrawerDepth", depthOutput);
        //プロパティ名は違うけど、渡す情報は、スポットライトと同じ。
    }

    public void Draw(Drawable drawable)
    {
        drawable.Draw(drawingMat);
        //描き込みの処理自体は、Drawableで行う。
        //描き込むためのMaterialはProjectionSprayが持っている。
    }
}
//}

==== Drawable.cs

スプレーで描き込まれる対象のオブジェクト。描き込むためのテクスチャを持っています。
@<code>{Start()}関数内で@<code>{RenderTexture}を作成しています。クラシックなPing-pong Bufferを使用しています。

テクスチャに描き込みを行う部分の処理を見ていきましょう

//listnum[spray-drawable][Drawable.cs]{
    //この関数は、projectionSpray.Draw(Drawable drawable)から呼ばれる
    public void Draw(Material drawingMat)
    {
        drawingMat.SetTexture("_MainTex", pingPongRts[0]);
        //描き込む対象のテクスチャの現状をマテリアルに設定。

        var currentActive = RenderTexture.active;
        RenderTexture.active = pingPongRts[1];
        //描き込む対象のテクスチャを設定。
        GL.Clear(true, true, Color.clear);
        //描き込む対象のテクスチャをクリアする。
        drawingMat.SetPass(0);
        Graphics.DrawMeshNow(mesh, transform.localToWorldMatrix);
        //描き込む対象のメッシュとトランスフォーム値を使用してテクスチャを更新。
        RenderTexture.active = currentActive;

        Swap(pingPongRts);

        if(fillCrack!=null)
        {
            //Uvのつなぎ目にヒビができてしまうのを防ぐ処理です。
            Graphics.Blit(pingPongRts[0], pingPongRts[1], fillCrack);
            Swap(pingPongRts);
        }

        Graphics.CopyTexture(pingPongRts[0], output);
        //更新した後のテクスチャをoutputにコピー
    }
//}

ここでのポイントは、@<code>{Graphics.DrawMeshNow(mesh, matrix)}を使って@<code>{RenderTexture}の更新を行なっているところです。（@<list>{spray-drawable}）
@<code>{drawingMat}の頂点シェーダでメッシュの頂点を@<code>{mesh.uv2}の形状に展開しているので、メッシュの頂点位置や法線、トランスフォーム情報をフラグメントシェーダに渡した上で、テクスチャの更新が可能になっています。（@<list>{sprayProjection-shader}）

//listnum[sprayProjection-shader][ProjectionSpray.shader]{
v2f vert (appdata v)
{
    v.uv2.y = 1.0 - v.uv2.y;
    //yを反転しています！

    v2f o;
    o.vertex = float4(v.uv2*2.0 - 1.0, 0.0, 1.0);
    //showUv2と同じ処理です！
    o.uv = v.uv2;
    o.worldPos = mul(unity_ObjectToWorld, v.vertex).xyz;
    o.normal = UnityObjectToWorldNormal(v.normal);
    return o;
}

sampler2D _MainTex;

uniform float4x4 _ProjMatrix, _WorldToDrawerMatrix;

sampler2D _Cookie, _DrawerDepth;
half4 _DrawerPos, _Color;
half _Emission;

half4 frag (v2f i) : SV_Target
{
    ///diffuse
    half3 to = i.worldPos - _DrawerPos.xyz;
    half3 dir = normalize(to);
    half dist = length(to);
    half atten = _Emission * dot(-dir, i.normal) / (dist * dist);

    ///spot cookie
    half4 drawerSpacePos = mul(
        _WorldToDrawerMatrix, 
        half4(i.worldPos, 1.0)
    );
    half4 projPos = mul(_ProjMatrix, drawerSpacePos);
    projPos.z *= -1;
    half2 drawerUv = projPos.xy / projPos.z;
    drawerUv = drawerUv * 0.5 + 0.5;
    half cookie = tex2D(_Cookie, drawerUv);
    cookie *= 
        0<drawerUv.x && drawerUv.x<1 && 
        0<drawerUv.y && drawerUv.y<1 && 0<projPos.z;

    ///shadow
    half drawerDepth = tex2D(_DrawerDepth, drawerUv).r;
    atten *= 1.0 - saturate(10 * abs(drawerSpacePos.z) - 10 * drawerDepth);
    //ここまでは、スポットライトの処理と同じです！

    i.uv.y = 1 - i.uv.y;
    half4 col = tex2D(_MainTex, i.uv);
    //_MainTexには、drawable.pingPongRts[0]が割り当てられています
    col.rgb = lerp(
        col.rgb, 
        _Color.rgb, 
        saturate(col.a * _Emission * atten * cookie)
    );
    //ここが、描き込みを行う処理です！
    //計算したライティングの強度によって元のテクスチャから描き込みカラーに補完処理しています。

    col.a = 1;
    return col;
    //値は、drawable.pingPongRts[1]に出力されます
}
//}

これで、3Dモデルに対するスプレーによる描き込みができるようになりました。（@<img>{buddha-spray}）

//image[buddha-spray][01_projectionSpray.unity]{
//}

== まとめ

UnityCG.cgincやLighting.cgincなどを見るとビルトインの処理が書いてあって、いろいろな処理を実装する参考になるので見てみるとよいです！
