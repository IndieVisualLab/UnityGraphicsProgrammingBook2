
= GPU-Based Trail

== はじめに
本章では、GPU を活用して Trail（軌跡）を作る方法を紹介します。
本章のサンプルは @<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming2} の「GPUBasedTrail」です。

=== Trail（軌跡）とは

移動する物体の軌跡をTrailと呼んでいます。
広義の意味では車の轍や船の航跡、スキーのシュプールなども含みますが、
CG で印象的なものは車のテールランプやシューティングゲームのホーミングレーザーのような曲線を描く光跡表現ではないでしょうか。

=== Unity標準の Trail

Unityには標準で２種類のTrailが用意されています。  

  * @<b>{TrailRenderer}@<fn>{trailrenderer} GameObject の軌跡を描くために使われます
  * @<b>{Trails module}@<fn>{trailmodule}  Particle の軌跡を描くために使われます
//footnote[trailrenderer][https://docs.unity3d.com/ja/current/Manual/class-TrailRenderer.html]
//footnote[trailmodule][https://docs.unity3d.com/Manual/PartSysTrailsModule.html]

本章では Trail 自体の作り方に焦点を当てるため、あえてこれらの機能は使わず、
また GPU 上での実装にすることで Trails module 以上の物量表現を可能にします。

//image[fuqunaga/sample_image][サンプルコードの実行画面。10000 本の Trail を表示]


== データの作成

それでは Trail を作成していきましょう。

=== データの定義

使用する構造体は主に３つです。

//emlist[GPUTrails.cs]{
public struct Trail
{
  public int currentNodeIdx;
}
//}

Trail 構造体は１つが１本の Trail に対応します。
currentNodeIdx 最後に書き込みした Node バッファのインデックスを保存しています。

//emlist[GPUTrails.cs]{
public struct Node
{
  public float time;
  public Vector3 pos;
}
//}

Node 構造体は Trail 内の制御点です。
Node の位置と更新した時間を保存しています。

//emlist[GPUTrails.cs]{
public struct Input
{
  public Vector3 pos;
}
//}

Input 構造体はエミッタ（軌跡を残すもの）からの１フレーム分の入力です。ここでは位置だけですが、色などを追加しても面白いと思います。


=== 初期化

GPUTrails.Start() で使用するバッファを初期化していきます

//emlist[GPUTrails.cs]{
trailBuffer = new ComputeBuffer(trailNum, Marshal.SizeOf(typeof(Trail)));
nodeBuffer = new ComputeBuffer(totalNodeNum, Marshal.SizeOf(typeof(Node)));
inputBuffer = new ComputeBuffer(trailNum, Marshal.SizeOf(typeof(Input)));
//}

trailNum 個分の trailBufferを初期化しています。つまりこのプログラムでは複数本の Trail をまとめて処理しています。
nodeBuffer ではすべての Trail 分の Node をまとめて１つのバッファで扱っています。
インデックス０ ～ nodeNum-1 までが１本目、nodeNum ～ 2*nodeNum-1 までが２本目、といった具合です。
inputBuffer も trailNum 個保持し、全 Trail の入力を管理します。


//emlist[GPUTrails.cs]{
var initTrail = new Trail() { currentNodeIdx = -1 };
var initNode = new Node() { time = -1 };

trailBuffer.SetData(Enumerable.Repeat(initTrail, trailNum).ToArray());
nodeBuffer.SetData(Enumerable.Repeat(initNode, totalNodeNum).ToArray());
//}

各バッファに初期値を入れています。
Trail.currentNodeIdx、Node.time を負数にしておき、あとでこれらを未使用かどうかの判定に使います。
inputBuffer は最初の更新ですべて値が書き込まれるので初期化の必要がなくノータッチです。


=== Node バッファの使い方
ここで Node バッファの使い方について解説します。

==== 初期状態
//image[fuqunaga/node0][初期状態][scale=0.5]
まだ何も入力されていない状態です。

==== 入力中
//image[fuqunaga/node_update][入力中][scale=0.5]
１ノードづつ入力されて行きます。まだ未使用の Node があります。

==== ループ
//image[fuqunaga/node_loop][ループ][scale=0.5]
すべての Node を使い尽くすと、はじめに戻りの Node を上書きして行きます。リングバッファ状に使用しています。


=== インプット

ここからは毎フレーム呼ばれる処理になります。
エミッタの位置を入力して、Node を追加、更新していきます。

まずは外部で inputBuffer を更新していきます。
これはどんな処理でもかまいません。
はじめは CPU で計算して @<code>{ComputeBuffer.SetData()} するのが簡単で良いかもしれません。
サンプルコードでは簡単な GPU 実装のパーティクルを動かしこれらをエミッタとして扱っています。

===[column] Curl Noise
サンプルコードのパーティクルは、Curl Noise で受ける力を求めて移動する挙動にしています。
このように Curl Noise は簡単に疑似流体っぽい動きを作れたりするのでとても便利です。
本書の@<chapref>{sakota}で@<b>{@sakope}さんが詳しく解説しているのでぜひ御覧ください。

===[/column]

==== エミッタの更新

//emlist[GPUTrailParticles.cs]{
void Update()
{
 cs.SetInt(CSPARAM.PARTICLE_NUM, particleNum);
 cs.SetFloat(CSPARAM.TIME, Time.time);
 cs.SetFloat(CSPARAM.TIME_SCALE, _timeScale);
 cs.SetFloat(CSPARAM.POSITION_SCALE, _positionScale);
 cs.SetFloat(CSPARAM.NOISE_SCALE, _noiseScale);
 
 var kernelUpdate = cs.FindKernel(CSPARAM.UPDATE);
 cs.SetBuffer(kernelUpdate, CSPARAM.PARTICLE_BUFFER_WRITE, _particleBuffer);
 
 var updateThureadNum = new Vector3(particleNum, 1f, 1f);
 ComputeShaderUtil.Dispatch(cs, kernelUpdate, updateThureadNum);
 
 
 var kernelInput = cs.FindKernel(CSPARAM.WRITE_TO_INPUT);
 cs.SetBuffer(kernelInput, CSPARAM.PARTICLE_BUFFER_READ, _particleBuffer);
 cs.SetBuffer(kernelInput, CSPARAM.INPUT_BUFFER, trails.inputBuffer);
 
 var inputThreadNum = new Vector3(particleNum, 1f, 1f);
 ComputeShaderUtil.Dispatch(cs, kernelInput, inputThreadNum);
}
//}

２つのカーネルを実行しています。

 : CSPARAM.UPDATE
 エミッタとして使用するパーティクルを更新しています。


 : CSPARAM.WRITE_TO_INPUT
 エミッタの現在の位置を inputBuffer に書き込んでいます。
 これを Trail の入力して使用します。


==== Trail への入力

さて、それでは inputBuffer を参照して、nodeBuffer を更新しましょう。

//emlist[GPUTrailParticles.cs]{
void LateUpdate()
{
  cs.SetFloat(CSPARAM.TIME, Time.time);
  cs.SetFloat(CSPARAM.UPDATE_DISTANCE_MIN, updateDistaceMin);
  cs.SetInt(CSPARAM.TRAIL_NUM, trailNum);
  cs.SetInt(CSPARAM.NODE_NUM_PER_TRAIL, nodeNum);

  var kernel = cs.FindKernel(CSPARAM.CALC_INPUT);
  cs.SetBuffer(kernel, CSPARAM.TRAIL_BUFFER, trailBuffer);
  cs.SetBuffer(kernel, CSPARAM.NODE_BUFFER, nodeBuffer);
  cs.SetBuffer(kernel, CSPARAM.INPUT_BUFFER, inputBuffer);

  ComputeShaderUtil.Dispatch(cs, kernel, new Vector3(trailNum, 1f, 1f));
}
//}

CPU 側では必要なパラメータを渡してComputeShaderをDispatch()しているだけです。
メインの ComputeShader 側の処理は次のようになっています。

//emlist[GPUTrail.compute]{
[numthreads(256,1,1)]
void CalcInput (uint3 id : SV_DispatchThreadID)
{
  uint trailIdx = id.x;
  if ( trailIdx < _TrailNum)
  {	
  	Trail trail = _TrailBuffer[trailIdx];
  	Input input = _InputBuffer[trailIdx];
  	int currentNodeIdx = trail.currentNodeIdx;
  
  	bool update = true;
  	if ( trail.currentNodeIdx >= 0 )
  	{
  	  Node node = GetNode(trailIdx, currentNodeIdx);
	  float dist = distance(input.position, node.position);
  	  update = dist > _UpdateDistanceMin;
  	}
  
  	if ( update )
  	{
  	  Node node;
  	  node.time = _Time;
  	  node.position = input.position;
    
  	  currentNodeIdx++;
  	  currentNodeIdx %= _NodeNumPerTrail;
    
  	  // write new node
  	  SetNode(node, trailIdx, currentNodeIdx);
    
  	  // update trail
  	  trail.currentNodeIdx = currentNodeIdx;
  	  _TrailBuffer[trailIdx] = trail;
  	}
  }
}
//}

くわしく見ていきましょう。

//emlist{
uint trailIdx = id.x;
if ( trailIdx < _TrailNum)
//}

まずは引数の id を Trail のインデックスとして使用しています。
スレッド数の関係で Trail 数以上の id で呼ばれてしまうこともあるので範囲外のものを if 文で弾いています。

//emlist{
int currentNodeIdx = trail.currentNodeIdx;

bool update = true;
if ( trail.currentNodeIdx >= 0 )
{
    Node node = GetNode(trailIdx, currentNodeIdx);
    update = distance(input.position, node.position) > _UpdateDistanceMin;
}
//}

次に @<code>{Trail.currentNodeIdx} を確認しています。
負数の場合は未使用の Trail です。

@<code>{ GetNode() }は、_NodeBuffer から指定の Node を取得する関数です。
インデックスの計算が間違いの元なので関数化しています。

すでに使用されている Trail では、最新の Node とインプット位置との距離を比較して、
@<code>{_UpdateDistanceMin} より離れていれば更新、近ければ更新しない、としています。
エミッタの挙動によりますが、前回の Node とほぼ同じ位置のインプットはたいていほぼ停止状態で微妙に誤差で移動している状態なので、
これらを律儀に Node 化して Trail を生成しようとすると、連続する Node 間で方向が大きく異なりかなり汚くなることが多いです。
したがってあまり近い距離ではあえて Node の追加をせずにスキップしています。


//emlist[GPUTrail.compute]{
if ( update )
{
  Node node;
  node.time = _Time;
  node.position = input.position;
  
  currentNodeIdx++;
  currentNodeIdx %= _NodeNumPerTrail;
  
  // write new node
  SetNode(node, trailIdx, currentNodeIdx);
  
  // update trail
  trail.currentNodeIdx = currentNodeIdx;
  _TrailBuffer[trailIdx] = trail;
}
//}

最後に _NodeBuffer および _TrailBuffer を更新しています。
Trail には入力した Node のインデックスを currentNodeIdx として保存します。Trail あたりの Node 数を超えたらリングバッファ状になるようゼロに戻しています。
Node には入力の時間と位置を保存しています。

さて、これで Trail の論理的な処理は完成です。
次はこの情報から描画する処理について見ていきましょう。


== 描画

Trail の描画は基本的には Node 間をラインで繋いでいく処理になります。
ここでは個々の Trail はできるだけ簡素にして物量を重視していこうと思います。
そのためラインはできるだけポリゴン数を少なくしたいのでカメラと正対する板ポリゴンとして生成します。

=== カメラと正対する板ポリゴンの生成

カメラと正対する板ポリゴンを生成する方法は次のようになります。

//image[fuqunaga/polygon0][Node 列][scale=0.7]

このような Node 列から

//image[fuqunaga/polygon1][Node から生成した頂点][scale=0.7]

各 Node から視線方向と垂直な方向に指定の幅だけ移動した頂点を求めます。

//image[fuqunaga/polygon2][ポリゴン化][scale=0.7]

生成した頂点同士を繋いでポリゴンにします。
それでは実際のコードを見ていきましょう。


=== CPU 側

CPU 側は単純にパラメータをマテリアルに渡して DrawProcedual() するだけの処理になっています。

//emlist[GPUTrailRenderer.cs]{
void OnRenderObject()
{
  _material.SetInt(GPUTrails.CSPARAM.NODE_NUM_PER_TRAIL, trails.nodeNum);
  _material.SetFloat(GPUTrails.CSPARAM.LIFE, trails._life);
  _material.SetBuffer(GPUTrails.CSPARAM.TRAIL_BUFFER, trails.trailBuffer);
  _material.SetBuffer(GPUTrails.CSPARAM.NODE_BUFFER, trails.nodeBuffer);
  _material.SetPass(0);
  
  var nodeNum = trails.nodeNum;
  var trailNum = trails.trailNum;
  Graphics.DrawProcedural(MeshTopology.Points, nodeNum, trailNum);
}
//}

いままで出てこなかったパラメータ@<code>{trails._life}が登場しています。
これは Node の生存時間で Node 自身が持っている生成時刻と照らし合わせて、これだけの時間が経つと透明にしていくような処理に使います。
こうすることで Trail の末端がなめらかに消えていく表現ができます。

特に入力すべきメッシュやポリゴンもないので、
@<code>{Graphics.DrawProcedural()}で trails.nodeNum 個の頂点あるモデルを trails.trailNum 個のインスタンスまとめて描画する命令を発行しています。


=== GPU 側

==== vertex shader

//emlist[GPUTrails.shader]{
vs_out vert (uint id : SV_VertexID, uint instanceId : SV_InstanceID)
{
  vs_out Out;
  Trail trail = _TrailBuffer[instanceId];
  int currentNodeIdx = trail.currentNodeIdx;
  
  Node node0 = GetNode(instanceId, id-1);
  Node node1 = GetNode(instanceId, id); // current
  Node node2 = GetNode(instanceId, id+1);
  Node node3 = GetNode(instanceId, id+2);
  
  bool isLastNode = (currentNodeIdx == (int)id);
  
  if ( isLastNode || !IsValid(node1))
  {
  	node0 = node1 = node2 = node3 = GetNode(instanceId, currentNodeIdx);
  }
  
  float3 pos1 = node1.position;
  float3 pos0 = IsValid(node0) ? node0.position : pos1;
  float3 pos2 = IsValid(node2) ? node2.position : pos1;
  float3 pos3 = IsValid(node3) ? node3.position : pos2;
  
  Out.pos = float4(pos1, 1);
  Out.posNext = float4(pos2, 1);
  
  Out.dir = normalize(pos2 - pos0);
  Out.dirNext = normalize(pos3 - pos1);
  
  float ageRate = saturate((_Time.y - node1.time) / _Life);
  float ageRateNext = saturate((_Time.y - node2.time) / _Life);
  Out.col = lerp(_StartColor, _EndColor, ageRate);
  Out.colNext = lerp(_StartColor, _EndColor, ageRateNext);
  
  return Out;
}
//}

まずは vertex shader の処理です。
このスレッドに対応した現在の Node とその次の Node の情報を出力します。

//emlist[GPUTrails.shader]{
Node node0 = GetNode(instanceId, id-1);
Node node1 = GetNode(instanceId, id); // current
Node node2 = GetNode(instanceId, id+1);
Node node3 = GetNode(instanceId, id+2);
//}

現在の Node を node1 として、１つ前の node0、１つ先の node2、２つ先の node3 と計４つの Node を参照しています。

//emlist[GPUTrails.shader]{
bool isLastNode = (currentNodeIdx == (int)id);

if ( isLastNode || !IsValid(node1))
{
  node0 = node1 = node2 = node3 = GetNode(instanceId, currentNodeIdx);
}
//}

現在の Node が末端であるか、まだ未入力である場合、node0～3 を末端の Node のコピーとして扱います。
つまり末端より先のまだ情報が無い Node をすべて末端に"折りたたんでいる"扱いにしています。
こうすることでこのあとのポリゴン生成の処理にそのまま流すことができます。

//emlist[GPUTrails.shader]{
float3 pos1 = node1.position;
float3 pos0 = IsValid(node0) ? node0.position : pos1;
float3 pos2 = IsValid(node2) ? node2.position : pos1;
float3 pos3 = IsValid(node3) ? node3.position : pos2;

Out.pos = float4(pos1, 1);
Out.posNext = float4(pos2, 1);
//}

さて、４つの Node から位置情報を取り出します。
現在の Node （node1)以外はすべて未入力である可能性があるので注意が必要です。
node0 が未入力なケースがちょっと意外かもしれませんが、
currentNodeIdx == 0 のときリングバッファを遡って node0 はバッファの一番最後の Node を指しているのでこのようなケースがありえます。
この場合も node1 の位置をコピーすることで、同じ場所に折りたたみます。
node2,3 も同様です。
このうち、pos1、pos2 を geometry shader に向けて出力します。

//emlist[GPUTrails.shader]{
Out.dir = normalize(pos2 - pos0);
Out.dirNext = normalize(pos3 - pos1);
//}

さらに pos0 → pos2 の方向ベクトルを pos1 における接線（tangent）、
pos1 → pos3 の方向ベクトルを pos2 における tangent として出力します。

//emlist[GPUTrails.shader]{
float ageRate = saturate((_Time.y - node1.time) / _Life);
float ageRateNext = saturate((_Time.y - node2.time) / _Life);
Out.col = lerp(_StartColor, _EndColor, ageRate);
Out.colNext = lerp(_StartColor, _EndColor, ageRateNext);
//}

最後に node1、node2 の書き込み時間と現在の時間を比較して色を求めています。

==== geometry shader

//emlist[GPUTrails.shader]{
[maxvertexcount(4)]
void geom (point vs_out input[1], inout TriangleStream<gs_out> outStream)
{
  gs_out output0, output1, output2, output3;
  float3 pos = input[0].pos; 
  float3 dir = input[0].dir;
  float3 posNext = input[0].posNext; 
  float3 dirNext = input[0].dirNext;
  
  float3 camPos = _WorldSpaceCameraPos;
  float3 toCamDir = normalize(camPos - pos);
  float3 sideDir = normalize(cross(toCamDir, dir));
  
  float3 toCamDirNext = normalize(camPos - posNext);
  float3 sideDirNext = normalize(cross(toCamDirNext, dirNext));
  float width = _Width * 0.5;
  
  output0.pos = UnityWorldToClipPos(pos + (sideDir * width));
  output1.pos = UnityWorldToClipPos(pos - (sideDir * width));
  output2.pos = UnityWorldToClipPos(posNext + (sideDirNext * width));
  output3.pos = UnityWorldToClipPos(posNext - (sideDirNext * width));
  
  output0.col =
  output1.col = input[0].col;
  output2.col =
  output3.col = input[0].colNext;
  
  outStream.Append (output0);
  outStream.Append (output1);
  outStream.Append (output2);
  outStream.Append (output3);
  
  outStream.RestartStrip();
}
//}

次に geometry shader の処理です。
vertex shader から渡された Node ２つ分の情報からいよいよポリゴンを生成します。
２つの pos と dir から、４つの位置＝４角形を求め TriangleStream として出力しています。

//emlist[GPUTrails.shader]{
float3 camPos = _WorldSpaceCameraPos;
float3 toCamDir = normalize(camPos - pos);
float3 sideDir = normalize(cross(toCamDir, dir));
//}

pos からカメラへの方向ベクトル(toCameraDir)と、接線ベクトル(dir)の外積を求め、これをラインの幅として広げる方向(sideDir)にしています。

#@#図がほしい


//emlist[GPUTrails.shader]{
output0.pos = UnityWorldToClipPos(pos + (sideDir * width));
output1.pos = UnityWorldToClipPos(pos - (sideDir * width));
//}

正負の sideDir 方向に移動した頂点を求めます。ここで Clip 座標系にして fragment shader へ渡すための座標変換まで済ませておきます。
posNext に関しても同じ処理をすることで計４つの頂点が求まりました。

#@# コラム幅を正しく求めるには

//emlist[GPUTrails.shader]{
output0.col =
output1.col = input[0].col;
output2.col =
output3.col = input[0].colNext;
//}

各頂点に色を乗せて完成です。

==== fragment shader

//emlist[GPUTrails.shader]{
fixed4 frag (gs_out In) : COLOR
{
  return In.col;
}
//}

最後に fragment shader です。
これ以上無いくらい単純です。
色を出力してるだけですね（笑


== 応用

以上で Trail の生成ができたかと思います。
今回は色だけの処理でしたが、テクスチャをのせたり、幅を変えてみたりといろいろな応用ができると思います。
また、GPUTrails.cs、GPUTRailsRenderer.cs とソースコードも別れているとおり、GPUTrails.shader 側は単なるバッファを見て描画するだけの処理なので
_TrailBuffer、_NodeBuffer さえ用意すれば実は Trail に限らずライン状の表示に流用できます。
今回は _NodeBuffer に追加するだけの Trail でしたが、毎フレーム全 Node を更新することで触手のようなウネウネしたものも表現したりできると思います。


== まとめ

本章では Trail の GPU 実装のできるだけシンプルな例を紹介しました。
GPU を使うとデバッグが大変になる反面、 CPU ではできないような圧倒的な物量表現が可能になります。
その「うひょー！」感を本書を通して体験できる方が一人でも増えたらいいなと思います。
また、Trail は「モデルを表示する」「スクリーンスペースでアルゴリズムで描画する」の間くらいの応用の幅が広く面白い領域の表現ではないかと思います。
この過程で得る理解は Trail に限らずいろいろな映像表現をプログラミングするときに役立つのではないかと思います。
