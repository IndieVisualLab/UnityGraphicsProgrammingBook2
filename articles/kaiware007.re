= ライン表現のためのGeometryShaderの応用

== はじめに

今年は個人でArt Hack Day 2018@<fn>{kaiware_arthackday}というアート作品を制作するハッカソンに参加しまして、そこでUnityを使ったビジュアル作品を制作しました。
//image[Kaiware/kaiware_arthackday2018][Already Thereのビジュアル部分][scale=0.5]

作品内で、Geometry Shaderを使ってワイヤーフレームの多角形を描画するという手法を使いました。本章ではその手法の解説を行っていきます。本章のサンプルは @<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming2} の「GeometryWireframe」です。


//footnote[kaiware_arthackday][Art Hack Day 2018 http://arthackday.jp/]

== とりあえず線を書いてみる

Unityで線を引くには、LineRendererやGLを使うことが多いと思いますが、今回は後々描画量が増えることを想定して、Graphics.DrawProceduralを使って行くことにします。@<br>{}
まず最初に、簡単なSin波を描画してみます。
サンプルの@<b>{SampleWaveLineシーン}を見てください。
//image[Kaiware/kaiware_project_sinwave001][SampleWaveLineシーン]
とりあえず、再生ボタンを押して実行すると、Gameビューにオレンジ色のSin波が表示されるはずです。
HierarchyウィンドウのWabeLineオブジェクトを選択し、InspectorウィンドウでRenderWaveLineコンポーネントのVertex Numのスライダーを動かすと、Sin波の滑らかさが変化します。
そのRenderWaveLineクラスの実装は次のようになっています。
//list[kaiware_render_waveline_cs][RenderWaveLine.cs]{
using UnityEngine;

[ExecuteInEditMode]
public class RenderWaveLine : MonoBehaviour {
    [Range(2,50)]
    public int vertexNum = 4;

    public Material material;

    private void OnRenderObject()
    {
        material.SetInt("_VertexNum", vertexNum - 1);
        material.SetPass(0);
        Graphics.DrawProcedural(MeshTopology.LineStrip, vertexNum);
    }
}
//}
Graphics.DrawProceduralは呼び出し後すぐに実行されるので、OnRenderObjectの中で呼ばなければなりません。
OnRenderObjectは、全てのカメラがシーンをレンダリングした後に呼び出されます。
Graphics.DrawProceduralの第１引数は@<b>{MeshTopology}です。MeshTopologyはメッシュをどのように構成するかの指定です。指定できる構成は、Triangles（三角ポリゴン）,Quads（四角ポリゴン）,Lines（2点をつなぐ線）,LineStrip（全ての点を連続してつなぐ）,Points（独立した点）の６つです。第２引数は@<b>{頂点数}です。@<br>{}
今回は、Sin波の線上に頂点を配置して、線を結ぶようにしたいため、@<b>{MeshTopology.LineStrip}を使います。
第２引数のvertexNumは、sin波を描画するのに使う頂点数を指定しています。
ここでカンのよい方なら気づくかもしれませんが、どこにも頂点座標の配列をShaderに渡していません。
頂点座標は次のShaderのVertex Shader（頂点シェーダ）の中で計算しています。
次にWaveLine.shaderです。
//list[kaiware_waveline_shader][WaveLine.shader]{
Shader "Custom/WaveLine"
{
  Properties
  {
    _Color ("Color", Color) = (1,1,1,1)
    _ScaleX ("Scale X", Float) = 1
    _ScaleY ("Scale Y", Float) = 1
    _Speed ("Speed",Float) = 1
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
      #pragma target 3.5

      #include "UnityCG.cginc"

      #define PI  3.14159265359

      struct v2f
      {
        float4 vertex : SV_POSITION;
      };

      float4 _Color;
      int _VertexNum;
      float _ScaleX;
      float _ScaleY;
      float _Speed;

      v2f vert (uint id : SV_VertexID)
      {
        float div = (float)id / _VertexNum;
        float4 pos = float4((div - 0.5) * _ScaleX,
          sin(div * 2 * PI + _Time.y * _Speed) * _ScaleY, 0, 1);

        v2f o;
        o.vertex = UnityObjectToClipPos(pos);
        return o;
      }

      fixed4 frag (v2f i) : SV_Target
      {
        return _Color;
      }
      ENDCG
    }
  }
}
//}

Vertex Shaderの関数vertの引数にSV_VertexID（頂点ID）が渡ってくるようになっています。
頂点IDは、頂点固有の通し番号です。感覚的には、Graphics.DrawProceduralの第２引数に使用する頂点数を渡すと、Vertex Shaderが頂点数の回数分呼び出され、引数の頂点IDには0～頂点数-1までの値が入る感じです。@<br>{}
Vertex Shader内では、頂点IDを頂点数で割ることで0～1までの割合を計算しています。その求めた割合をもとに頂点座標（pos）を計算しています。Y座標の計算で先ほど求めた割合をsin関数に与えてsin波上の座標を求めています。ついでに_Time.yを足すことで時間の進行による高さの変化のアニメーションも行っています。Vertex Shader内で頂点座標を計算しているのでC#側から頂点座標を渡す必要がないのです。それから、UnityObjectToClipPosでオブジェクト空間からカメラのクリップ空間へ変換した座標をFragment Shaderに渡しています。

== Geometry Shaderで動的に二次元の多角形を描く

=== Geometry Shaderで頂点を増やす

次に多角形を描画してみます。多角形を描画するにはそれぞれの角の分だけ頂点が必要です。前項のように頂点をつないで閉じたらできてしまいますが、今回はGeometry Shaderを使って１個の頂点から多角形を描画してみます。Geometry Shaderの詳細は、UnityGraphicsProgramming vol.1@<fn>{kaiware_unitygraphicsprograming}の「第6章 ジオメトリシェーダーで草を生やす」を参照してください。ざっくり解説すると、Geometry Shaderは、Vertex ShaderとFragment Shaderの間に位置する、頂点を増やすことができるシェーダです。

サンプルの@<b>{SamplePolygonLineシーン}を見てください。
//image[Kaiware/kaiware_project_polygonline001][SamplePolygonLineシーン]
再生ボタンを押して実行すると、Gameビュー上で三角形が回転が回転しているはずです。HierarchyウィンドウのPolygonLineオブジェクトを選択し、InspectorウィンドウでSinglePolygon2DコンポーネントのVertex Numのスライダーを動かすと、三角形の角数が増減できます。
そのSimglePolygon2Dクラスの実装は次のようになっています。
//list[kaiware_single_polygon_2d_cs][SinglePolygon2D.cs]{
  using UnityEngine;

[ExecuteInEditMode]
public class SinglePolygon2D : MonoBehaviour {

    [Range(2, 64)]
    public int vertexNum = 3;

    public Material material;

    private void OnRenderObject()
    {
        material.SetInt("_VertexNum", vertexNum);
        material.SetMatrix("_TRS", transform.localToWorldMatrix);
        material.SetPass(0);
        Graphics.DrawProcedural(MeshTopology.Points, 1);
    }
}
//}
RenderWaveLineクラスとほぼ同じ実装になっています。@<br>{}
大きく違う点が２つあります。ひとつ目は、Graphics.DrawProceduralの第１引数が@<b>{MeshTopology.LineStrip}から@<b>{MeshTopology.Points}になっている点です。もうひとつは、Graphics.DrawProceduralの第２引数が@<b>{１}固定になっている点です。前項のRenderWaveLineクラスは、頂点同士をつないで線を引いていたので、@<b>{MeshTopology.LineStrip}を指定していましたが、今回は１個の頂点だけ渡して多角形を描画したいので、@<b>{MeshTopology.Points}を指定しています。というのも、MeshTopologyの指定によって、描画に最低限必要な頂点数が変わり、それを下回っていると何も描画されません。MeshTopology.LinesとMeshTopology.LineStripは線なので２、MeshTopology.Trianglesは三角形なので3、MeshTopology.Pointsは点なので１です。
ちなみに、material.SetMatrix("_TRS", transform.localToWorldMatrix); の部分ですが、SinglePolygon2Dコンポーネントを割り当てているGameObjectのローカル座標系からワールド座標系へ変換した行列をシェーダーに渡しています。これをシェーダー内で頂点座標に掛けることで、GameObjectのtransform、すなわち、座標（position）、向き（rotation）、大きさ（scale）が描画する図形に反映されます。

続いてSinglePolygonLine.Shaderの実装を見てみましょう。

//list[kaiware_single_polygon_line_shader][SinglePolygonLine.shader]{
Shader "Custom/Single Polygon Line"
{
  Properties
  {
    _Color ("Color", Color) = (1,1,1,1)
    _Scale ("Scale", Float) = 1
    _Speed ("Speed",Float) = 1
  }
  SubShader
  {
    Tags { "RenderType"="Opaque" }
    LOD 100

    Pass
    {
      CGPROGRAM
      #pragma vertex vert
      #pragma geometry geom  // Geometry Shader の宣言
      #pragma fragment frag
      #pragma target 4.0

      #include "UnityCG.cginc"

      #define PI  3.14159265359

      // 出力構造体
      struct Output
      {
        float4 pos : SV_POSITION;
      };

      float4 _Color;
      int _VertexNum;
      float _Scale;
      float _Speed;
      float4x4 _TRS;

      Output vert (uint id : SV_VertexID)
      {
        Output o;
        o.pos = mul(_TRS, float4(0, 0, 0, 1));
        return o;
      }

      // ジオメトリシェーダ
      [maxvertexcount(65)]
      void geom(point Output input[1], inout LineStream<Output> outStream)
      {
        Output o;
        float rad = 2.0 * PI / (float)_VertexNum;
        float time = _Time.y * _Speed;

        float4 pos;

        for (int i = 0; i <= _VertexNum; i++) {
          pos.x = cos(i * rad + time) * _Scale;
          pos.y = sin(i * rad + time) * _Scale;
          pos.z = 0;
          pos.w = 1;
          o.pos = UnityObjectToClipPos(pos);

          outStream.Append(o);
        }
        outStream.RestartStrip();
      }

      fixed4 frag (Output i) : SV_Target
      {
        return _Color;
      }
      ENDCG
    }
  }
}
//}
@<b>{#pragma vertex vert}と@<b>{#pragma fragment frag}の間に、新たに@<b>{#pragma geometry geom}の宣言が追加されています。これは、geomという名前のGeometry Shaderの関数を宣言するという意味です。
Vertex Shaderのvertは、今回は頂点の座標をとりあえず原点（0,0,0,1）にして、それにC#から渡された_TRS行列（ローカル座標系からワールド座標系へ変換する行列）を掛けるようになっています。多角形の各頂点の座標計算は次のGeometry Shaderの中で行います。

//emlist[Geometry Shaderの定義]{
  // ジオメトリシェーダ
  [maxvertexcount(65)]
  void geom(point Output input[1], inout LineStream<Output> outStream)
//}
==== maxvertexcount
Geometry Shaderから出力する頂点の最大数です。今回は、SinglePolygonLineクラスのVertexNumで64頂点まで増やせるようにしていますが、64個目の頂点から0個目の頂点を結ぶ線が必要な為、65を指定しています。

==== point Output input[1]

Vertex Shaderからの入力情報を表しています。pointはprimitiveTypeで頂点１個分受け取るという意味で、Outputは構造体名、input[1]は長さ１の配列を表しています。今回は１つの頂点しか使わないのでpointとinput[1]を指定しましたが、メッシュなど三角ポリゴンの頂点をいじりたい時はtriangleとinput[3]にしたりします。

==== inout LineStream<Output> outStream

Geometry Shaderからの出力情報を表しています。LineStream<Output>は、Output構造体の線を出力するという意味です。他にもPointStream（点）、TriangleStream（三角ポリゴン）があります。
次に関数内の説明です。

//emlist[関数内の実装]{
Output o;
float rad = 2.0 * PI / (float)_VertexNum;
float time = _Time.y * _Speed;

float4 pos;

for (int i = 0; i <= _VertexNum; i++) {
  pos.x = cos(i * rad + time) * _Scale;
  pos.y = sin(i * rad + time) * _Scale;
  pos.z = 0;
  pos.w = 1;
  o.pos = UnityObjectToClipPos(pos);

  outStream.Append(o);
}

outStream.RestartStrip();
//}
多角形の各頂点の座標を計算するために、２π（360度）を頂点数で割って、一角の角度を求めています。それをループ内で三角関数（sin, cos）を使って頂点座標を計算しています。計算した座標をoutStream.Append(o)で頂点として出力します。_VertexNumの数だけループを回して頂点を出力したあと、outStream.RestartStrip()で現在のストリップを終了して次のストリップを開始します。Append()で追加していく限り、LineStreamとして線が繋がっていきます。RestartStrip()を実行することで一旦現在の線を終了します。次にAppend()が呼ばれると、前の線とは繋がらず、新しい線が始まります。

//footnote[kaiware_unitygraphicsprograming][UnityGraphicsProgramming vol.1 https://indievisuallab.stores.jp/items/59edf11ac8f22c0152002588]

== Octahedron Sphereを作ってみる

=== Octahedron Sphereとは？

正八面体（Regular octahedron）とは、@<img>{Kaiware/kaiware_octahedron001}にあるとおり、８つの正三角形で構成された多面体です。Octahedron Sphereとは、正八面体を構成する正三角形の３つの頂点を球面線形補間@<fn>{kaiware_slerp}して分割していくことで作られる球体です。通常の線形補間が２点間を直線でつなぐように補間するのに対し、球面線形補間は、@<img>{Kaiware/kaiware_slerp_001}のように２点間を球面上を通るように補間します。
//image[Kaiware/kaiware_octahedron001][正八面体][scale=1]
//image[Kaiware/kaiware_slerp_001][正八面体][scale=1]

サンプルの@<b>{SampleOctahedronSampleシーン}を見てください。
//image[Kaiware/kaiware_project_octahedronsphere001][SampleWaveLineシーン][scale=1]
実行ボタンを押すと、Gameビューの中央にゆっくり回転する正八面体が表示されているはずです。
また、HierarchyウィンドウのSingleOctahedronSphereオブジェクトのGeometry Octahedron SphereコンポーネントのLevelのスライダーを変更すると、正八面体の辺が分割されて少しずつ球体に近づいていくと思います。

//footnote[kaiware_slerp][spherical linear interpolation, 略してslerp]

=== Geometry Shaderの中で正八面体を分割していく

次に、実装を見てみましょう。C#側の実装は、前項のSinplePolygon2D.csとほぼほぼ同じなので省略します。OctahedronSphere.shaderは、ソース全体は長いので、Geometry Shaderの中だけ解説していきます。

//list[kaiware_octahedron_sphere_1][OctahedronSphere.shaderのGometry Shaderの先頭部分]{
// ジオメトリシェーダ
  float4 init_vectors[24];
  // 0 : the triangle vertical to (1,1,1)
  init_vectors[0] = float4(0, 1, 0, 0);
  init_vectors[1] = float4(0, 0, 1, 0);
  init_vectors[2] = float4(1, 0, 0, 0);
  // 1 : to (1,-1,1)
  init_vectors[3] = float4(0, -1, 0, 0);
  init_vectors[4] = float4(1, 0, 0, 0);
  init_vectors[5] = float4(0, 0, 1, 0);
  // 2 : to (-1,1,1)
  init_vectors[6] = float4(0, 1, 0, 0);
  init_vectors[7] = float4(-1, 0, 0, 0);
  init_vectors[8] = float4(0, 0, 1, 0);
  // 3 : to (-1,-1,1)
  init_vectors[9] = float4(0, -1, 0, 0);
  init_vectors[10] = float4(0, 0, 1, 0);
  init_vectors[11] = float4(-1, 0, 0, 0);
  // 4 : to (1,1,-1)
  init_vectors[12] = float4(0, 1, 0, 0);
  init_vectors[13] = float4(1, 0, 0, 0);
  init_vectors[14] = float4(0, 0, -1, 0);
  // 5 : to (-1,1,-1)
  init_vectors[15] = float4(0, 1, 0, 0);
  init_vectors[16] = float4(0, 0, -1, 0);
  init_vectors[17] = float4(-1, 0, 0, 0);
  // 6 : to (-1,-1,-1)
  init_vectors[18] = float4(0, -1, 0, 0);
  init_vectors[19] = float4(-1, 0, 0, 0);
  init_vectors[20] = float4(0, 0, -1, 0);
  // 7 : to (1,-1,-1)
  init_vectors[21] = float4(0, -1, 0, 0);
  init_vectors[22] = float4(0, 0, -1, 0);
  init_vectors[23] = float4(1, 0, 0, 0);
//}

まず、@<img>{Kaiware/kaiware_octahedron_triangle001}のように初期値となる“正規化された”正八面体の三角系を定義しています。
//image[Kaiware/kaiware_octahedron_triangle001][正八面体の頂点座標と三角形][scale=1]

float4で定義しているのは、クォータニオンとして定義しているためです。

//list[kaiware_octahedron_sphere_2][OctahedronSphere.shaderの三角形の球面線形補間分割処理部分]{
for (int i = 0; i < 24; i += 3)
{
  for (int p = 0; p < n; p++)
  {
    // edge index 1
    float4 edge_p1 = qslerp(init_vectors[i],
      init_vectors[i + 2], (float)p / n);
    float4 edge_p2 = qslerp(init_vectors[i + 1],
      init_vectors[i + 2], (float)p / n);
    float4 edge_p3 = qslerp(init_vectors[i],
      init_vectors[i + 2], (float)(p + 1) / n);
    float4 edge_p4 = qslerp(init_vectors[i + 1],
      init_vectors[i + 2], (float)(p + 1) / n);

    for (int q = 0; q < (n - p); q++)
    {
      // edge index 2
      float4 a = qslerp(edge_p1, edge_p2, (float)q / (n - p));
      float4 b = qslerp(edge_p1, edge_p2, (float)(q + 1) / (n - p));
      float4 c, d;

      if(distance(edge_p3, edge_p4) < 0.00001)
      {
        c = edge_p3;
        d = edge_p3;
      }
      else {
        c = qslerp(edge_p3, edge_p4, (float)q / (n - p - 1));
        d = qslerp(edge_p3, edge_p4, (float)(q + 1) / (n - p - 1));
      }

      output1.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, a));
      output2.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, b));
      output3.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, c));

      outStream.Append(output1);
      outStream.Append(output2);
      outStream.Append(output3);
      outStream.RestartStrip();

      if (q < (n - p - 1))
      {
        output1.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, c));
        output2.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, b));
        output3.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, d));

        outStream.Append(output1);
        outStream.Append(output2);
        outStream.Append(output3);
        outStream.RestartStrip();
      }
    }
  }
}
//}
三角形を球面線形補間で分割している部分です。ｎは三角形の分割数です。
edge_p1とedge_p2は三角形の開始点を、edge_p3とege_p4は、分割した辺の中点を求めています。qslerp関数は、球面線形補間を求める関数です。
qslerpの定義は次のとおりです。

//list[kaiware_octahedron_sphere_3][Quaternion.cgincのqslerpの定義]{
// a:開始Quaternion b:目標Quaternion t:比率
float4 qslerp(float4 a, float4 b, float t)
{
  float4 r;
  float t_ = 1 - t;
  float wa, wb;
  float theta = acos(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
  float sn = sin(theta);
  wa = sin(t_ * theta) / sn;
  wb = sin(t * theta) / sn;
  r.x = wa * a.x + wb * b.x;
  r.y = wa * a.y + wb * b.y;
  r.z = wa * a.z + wb * b.z;
  r.w = wa * a.w + wb * b.w;
  normalize(r);
  return r;
}
//}

==== 三角形分割の流れ１

続いて、三角形の分割処理の流れを説明します。
例として、分割数２（n=2）の場合の流れです。
//image[Kaiware/kaiware_triangle_div_001][三角形の分割処理の流れ１、edge_p1～p4の計算][scale=1]

@<img>{Kaiware/kaiware_triangle_div_001}は、次のコードを表しています。
//list[kaiware_octahedron_sphere_4][edge_p1～p4の計算]{
for (int p = 0; p < n; p++)
{
  // edge index 1
  float4 edge_p1 = qslerp(init_vectors[i],
    init_vectors[i + 2], (float)p / n);
  float4 edge_p2 = qslerp(init_vectors[i + 1],
    init_vectors[i + 2], (float)p / n);
  float4 edge_p3 = qslerp(init_vectors[i],
    init_vectors[i + 2], (float)(p + 1) / n);
  float4 edge_p4 = qslerp(init_vectors[i + 1],
    init_vectors[i + 2], (float)(p + 1) / n);
//}
@<b>{init_vectors}配列の３点から、edge_p1～edge_p4の座標を求めています。p=0の時は、p/n = 0/2 = 0でedge_p1＝init_vectors[0]、edge_p2=init_vectors[1]になります。edge_p3とedge_p4は、(p+1)/n = (0+1)/2 = 0.5でそれぞれinit_vectors[0]とinit_vectors[2]の間、init_vectors[1]とinit_vectors[2]の間になります。
主に三角形の右側を分割する流れです。

==== 三角形分割の流れ２

//image[Kaiware/kaiware_triangle_div_002][三角形の分割処理の流れ２、abcdの計算][scale=1]
@<img>{Kaiware/kaiware_triangle_div_002}は、次のコードを表しています。
//list[kaiware_octahedron_sphere_5][座標a,b,c,dの計算]{
for (int q = 0; q < (n - p); q++)
{
  // edge index 2
  float4 a = qslerp(edge_p1, edge_p2, (float)q / (n - p));
  float4 b = qslerp(edge_p1, edge_p2, (float)(q + 1) / (n - p));
  float4 c, d;

  if(distance(edge_p3, edge_p4) < 0.00001)
  {
    c = edge_p3;
    d = edge_p3;
  }
  else {
    c = qslerp(edge_p3, edge_p4, (float)q / (n - p - 1));
    d = qslerp(edge_p3, edge_p4, (float)(q + 1) / (n - p - 1));
  }
//}
前項で求めたedge_p1～p4を使って、頂点abcdの座標を求めています。主に三角形の左側を分割する流れです。条件によってはedge_p3とedge_p4の座標が同じになります。これは、三角形の右側がこれ以上分割できない段階になった時に発生します。その場合はc,dはどちらも三角形の右下の座標を取ります。

==== 三角形分割の流れ３

//image[Kaiware/kaiware_triangle_div_003][三角形の分割処理の流れ３、三角形abc,三角形cbdを出力][scale=1]
@<img>{Kaiware/kaiware_triangle_div_003}は、次のコードを表しています。
//list[kaiware_octahedron_sphere_6][座標a,b,cを結ぶ三角形＆座標c,b,dを結ぶ三角形を出力]{
output1.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, a));
output2.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, b));
output3.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, c));

outStream.Append(output1);
outStream.Append(output2);
outStream.Append(output3);
outStream.RestartStrip();

if (q < (n - p - 1))
{
  output1.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, c));
  output2.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, b));
  output3.pos = UnityObjectToClipPos(input[0].pos + mul(_TRS, d));
  outStream.Append(output1);
  outStream.Append(output2);
  outStream.Append(output3);
  outStream.RestartStrip();
}
//}
計算したa,b,c,dの座標を、UnityObjectToClipPosやワールド座標変換行列を掛けてスクリーン用の座標に変換します。
その後、outStream.AppendとoutStream.RestartStripで、a,b,cとc,b,dを結ぶ２つの三角形を出力します。

==== 三角形分割の流れ４

//image[Kaiware/kaiware_triangle_div_004][三角形の分割処理の流れ４、q=1の場合][scale=1]

q=1の場合、aは1/2=0.5なのでedge_p1とedge_p2の中間に、bは1/1=1なのでedge_p2の位置になります。
cは1/1=1なのでedge_p4に、dは一応計算してますが、if (q < (n - p - 1))の条件に引っかからないので使われません。a,b,cを結ぶ三角形を出力します。

==== 三角形分割の流れ５

//image[Kaiware/kaiware_triangle_div_005][三角形の分割処理の流れ５、p=1の場合][scale=1]
qのfor文が終わり、p=1になった時の流れです。p/n = 1/2 = 0.5なので、edge_p1はinit_vectors[0]とinit_vectors[2]の中間に、edge_p2はinit_vectors[1]とinit_vectors[2]の中間になります。あとのa,b,c,dの座標計算と三角形a,b,cの出力は前述の処理と同じです。これで１つの三角形を４つに分割できました。正八面体の全ての三角形に対して前述までを処理を行っています。

=== おまけ

この他にも、紙面の都合上、紹介しきれないサンプルを３つ用意していますので、興味がある方は是非ご覧になってください。

 * OctahedronSphereのGPUInstancing版（SampleOctahedronSphereInstancingシーン）
 * ９段階まで分割可能なOctahedronSphere単体版（SampleOctahedronSphereMultiVertexシーン）
 * ９段階まで分割可能なOctahedronSphereのGPUInstancing版（SampleOctahedronSphereMultiVertexInstancingシーン）

//image[Kaiware/kaiware_project_omake001][SampleOctahedronSphereMultiVertexInstancingシーン][scale=0.8]

== まとめ

本章では、ライン表現のためのGeometry Shaderの応用について解説しました。Geometry Shaderは普段はポリゴンを分割したり、パーティクルの板ポリゴンを作ったりという事例をよく見かけますが、皆さんも動的に頂点数を増やせる特性を利用して面白い表現を模索してみてください。
