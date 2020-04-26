
= Real-Time GPU-Based Voxelizer

== はじめに

本章では、GPUを活用してリアルタイムにメッシュのボクセル化を行うプログラム、GPUVoxelizerを開発します。

本章のサンプルは@<br>{}
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming2}@<br>{}
の「RealTimeGPUBasedVoxelizer」です。

まずはCPUでの実装をもとにボクセル化の手順と得られる結果について確認した後、
GPUでの実装方法を解説し、
高速なボクセル化を応用したエフェクト例を紹介します。

=== ボクセル（Voxel）とは

ボクセル（Voxel）は、3次元の正規格子空間における基本単位を表します。
2次元の正規格子空間の基本単位として用いられるピクセル（Pixel）の次元が1つ増えたものとしてイメージすることができ、
Volumeを持ったPixelという意味合いでVoxel（ボクセル）と命名されています。
ボクセルでは体積を表現することができ、各ボクセルに濃度などの値を格納したデータフォーマットを用意して、
医療や科学データの可視化や解析に用いられることがあります。

また、ゲームではMinecraft@<fn>{minecraft}がボクセルを利用したものとして挙げられます。

細かいモデルやステージを作り込むのは手間がかかりますが、
ボクセルモデルであれば比較的少ない手間で作ることができ、
フリーでもMagicaVoxel@<fn>{magicavoxel}などの優秀なエディターもあり、3Dドット絵のようにモデルを作成することができます。

//footnote[minecraft][https://minecraft.net]
//footnote[magicavoxel][http://ephtracy.github.io/]

== ボクセル化のアルゴリズム

ボクセル化のアルゴリズムについて、CPUでの実装をもとに解説していきます。
CPU実装はCPUVoxelizer.cs内に記述しています。

=== ボクセル化の大まかな流れ

ボクセル化の大まかな流れは以下になります。

 1. ボクセルの解像度を設定する
 2. ボクセル化を行う範囲を設定する
 3. ボクセルデータを格納する3次元の配列データを生成する
 4. メッシュの表面に位置するボクセルを生成する
 5. メッシュの表面を表すボクセルデータから、メッシュ内部に位置するボクセルを埋める

CPUでのボクセル化は、CPUVoxelizerクラスのstatic関数
//emlist[CPUVoxelizer.cs]{
public class CPUVoxelizer
{
    public static void Voxelize (
        Mesh mesh, 
        int resolution, 
        out List<Vector3> voxels, 
        out float unit,
        bool surfaceOnly = false
    ) {
    ...
    }
    ...
}
//}
をコールすることで実行します。
引数にボクセル化したいメッシュ、解像度を指定して実行すると、
ボクセル配列voxelsと、一つのボクセルのサイズを表すunitを参照引数経由で返します。

以下ではVoxelize関数の内部で行われていることを大まかな流れに沿って解説していきます。

=== ボクセルの解像度を設定する

ボクセル化を行うには、まず、ボクセルの解像度を設定します。
解像度が細かいほど小さなキューブでモデルを構築することになるので、詳細なボクセルモデルを生成できますが、
その分計算時間を必要とします。

//image[Nakamura/VoxelResolution][ボクセル解像度の違い]

=== ボクセル化を行う範囲を設定する

対象のメッシュモデルをボクセル化する範囲を指定します。
メッシュモデルが持つBoundingBox（モデルの頂点が全て収まる最小サイズの直方体）をボクセル化の範囲として指定すれば、
メッシュモデル全体をボクセル化することができます。

//image[Nakamura/BoundingBox][メッシュのBoundingBox]

ここで注意したいのは、メッシュモデルが持つBoundingBoxをそのままボクセル化の範囲として用いると、
Cubeメッシュのように、BoundingBoxにぴったり重なるような面を持つメッシュをボクセル化する際に問題が生じてしまいます。

詳しくは後述しますが、ボクセル化の際に、メッシュを構成する三角形とボクセルとの交差判定を行うのですが、
三角形とボクセルの面がぴったり重なる場合、正しく交差判定できない場合があります。

そのため、メッシュモデルが持つBoundingBoxを「一つのボクセルを構成する単位長の半分の長さ分」拡張した範囲を、ボクセル化の範囲として指定します。

//emlist[CPUVoxelizer.cs]{
mesh.RecalculateBounds();
var bounds = mesh.bounds;

// 指定された解像度から、一つのボクセルを構成する単位長を計算する
float maxLength = Mathf.Max(
    bounds.size.x, 
    Mathf.Max(bounds.size.y, bounds.size.z)
);
unit = maxLength / resolution;

// 単位長の半分の長さ
var hunit = unit * 0.5f;

// 「一つのボクセルを構成する単位長の半分の長さ分」拡張した範囲を
// ボクセル化の範囲とする

// ボクセル化するboundsの最小値
var start = bounds.min - new Vector3(hunit, hunit, hunit);

// ボクセル化するboundsの最大値
var end = bounds.max + new Vector3(hunit, hunit, hunit);

// ボクセル化するboundsのサイズ
var size = end - start;
//}

=== ボクセルデータを格納する3次元の配列データを生成する

ボクセルを表す構造体として、サンプルコードではVoxel_t構造体を用意しています。

//emlist[Voxel.cs]{
[StructLayout(LayoutKind.Sequential)]
public struct Voxel_t {
    public Vector3 position;   // ボクセルの位置
    public uint fill;          // ボクセルを埋めるべきかどうかのフラグ
    public uint front;         // ボクセルと交差した3角形が決められた方向から見て前面かどうかのフラグ
    ...
}
//}

このVoxel_tの三次元配列を生成し、そこにボクセルデータを格納していきます。

//emlist[CPUVoxelizer.cs]{
// ボクセルの単位長さとボクセル化を行う範囲に基づいて、3次元ボクセルデータのサイズを決定する
var width = Mathf.CeilToInt(size.x / unit);
var height = Mathf.CeilToInt(size.y / unit);
var depth = Mathf.CeilToInt(size.z / unit);
var volume = new Voxel_t[width, height, depth];
//}

また、後に続く処理の中で各ボクセルの位置やサイズを参照するため、事前に3次元ボクセルデータと一致するAABB配列を生成しておきます。
//emlist[CPUVoxelizer.cs]{
var boxes = new Bounds[width, height, depth];
var voxelUnitSize = Vector3.one * unit;
for(int x = 0; x < width; x++)
{
    for(int y = 0; y < height; y++)
    {
        for(int z = 0; z < depth; z++)
        {
            var p = new Vector3(x, y, z) * unit + start;
            var aabb = new Bounds(p, voxelUnitSize);
            boxes[x, y, z] = aabb;
        }
    }
}
//}

===[column] AABB

AABB（Axis-Aligned Bounding Box、軸平行境界ボックス）とは、
3次元空間のXYZ軸に対して各辺が平行な直方体の境界図形を指します。

AABBは衝突判定に利用されることが多く、
2つのメッシュ同士の衝突判定や、
あるメッシュと光線との衝突判定を簡易的に行う際に使われたりするケースがあります。

メッシュに対する衝突判定を厳密にやろうとすると、メッシュを構成する三角形全てに対して判定を行わなければなりませんが、
メッシュを含むAABBのみであれば高速に計算することができ、便利です。

//image[Nakamura/AABB][2つの多角形オブジェクトのAABB同士の衝突判定]

===[/column]

=== メッシュの表面に位置するボクセルを生成する

以下の図のように、メッシュの表面に位置するボクセルを生成します。

//image[Nakamura/VoxelSurface2][まずメッシュの表面に位置するボクセルを生成し、それを基にしてメッシュの中身を埋めていくようにボクセルを生成する]

メッシュの表面に位置するボクセルを求めるには、
メッシュを構成するそれぞれの3角形とボクセルとの交差判定を行う必要があります。

==== 3角形とボクセルの交差判定（SATを用いた交差判定アルゴリズム）

3角形とボクセルとの交差判定には、SAT（Separating Axis Theorem、分離軸定理）を用います。
SATを用いた交差判定アルゴリズムは3角形とボクセル同士に限らず、凸面同士の交差判定として汎用的に用いることができます。

SATは以下のことを証明しました。

===[column] SAT（Separating Axis Theorem、分離軸定理）の簡単な解説

一方にオブジェクトA全体、もう一方にオブジェクトB全体が存在するような直線が求められれば、
オブジェクトAとオブジェクトBは交わりません。
このような、2つのオブジェクトを分離する直線を分離直線と呼び、その分離直線は常に分離軸と直交します。

===[/column]

SATにより、2つの凸面同士の射影が重ならない軸（分離軸）が求められれば、その2つの凸面を分離する直線が存在するので、2つの凸面が交わらないということを導き出せます。
逆に、分離軸が見つからなければ2つの凸面同士は交差していると判断できます。
（形状が凹である場合、分離軸が見つからなくとも交差していない場合があります。）


凸面形状は、ある軸に射影されると、その形状の影がその軸を表す線の上に投影されたように映ります。
これは軸上の線分として表すことができ、範囲区間[min, max]で表わせます。

//image[Nakamura/SAT1][凸面形状をある軸に射影し、軸上に映された凸面形状の範囲（min, max）]

以下の図のように、
2つの凸面形状の分離直線が存在する場合、
その直線に直交する分離軸に対する凸面形状の射影区間同士は重なりません。

//image[Nakamura/SAT2][２つの凸面形状を分離する直線がある場合、その直線に直交する軸への射影区間同士は重ならない]

しかし同じ2つの凸面であっても、以下の図のように、
他の非分離軸への射影は重なることがあります。

//image[Nakamura/SAT3][2つの凸面形状を分離しない直線と直交する軸に射影した場合、射影同士が重なることがある]

形状によっては、分離軸になる可能性のある軸が明白であるものがあり、
そのような2つの形状AとBの間の交わりを判定するには、
分離軸になり得る軸それぞれに2つの形状を射影し、
2つの射影区間[Amin, Amax]と[Bmin, Bmax]が互いに重なっていないかどうかを調べていけば交差判定ができます。
式で表すと、Amax < Bmin または Bmax < Amin であれば、2つの区間は重なっていません。

凸面同士の分離軸になる可能性がある軸は

 * 凸面1の辺と凸面2の辺のクロス積
 * 凸面1の法線
 * 凸面2の法線

であることが導かれており、このことから三角形とボクセル（AABB）同士では、分離軸になる可能性がある軸は

 * 三角形の3つの辺とAABBの直交する3つの辺同士の組み合わせから得られる9通りのクロス積
 * AABBの3つの法線
 * 三角形の法線

であるため、これら13通りの軸それぞれについて、
射影が重なるかどうか判定していくことで、三角形とボクセルとの交差判定を行います。

一つの3角形とすべてのボクセルデータとの交差判定を行うのは無駄な処理が多くなる可能性があるため、
3角形を含むAABBを計算し、そこに含まれるボクセルとの交差判定を行います。

//emlist[CPUVoxelizer.cs]{
// 三角形のAABBを計算
var min = tri.bounds.min - start;
var max = tri.bounds.max - start;
int iminX = Mathf.RoundToInt(min.x / unit);
int iminY = Mathf.RoundToInt(min.y / unit);
int iminZ = Mathf.RoundToInt(min.z / unit);
int imaxX = Mathf.RoundToInt(max.x / unit);
int imaxY = Mathf.RoundToInt(max.y / unit);
int imaxZ = Mathf.RoundToInt(max.z / unit);
iminX = Mathf.Clamp(iminX, 0, width - 1);
iminY = Mathf.Clamp(iminY, 0, height - 1);
iminZ = Mathf.Clamp(iminZ, 0, depth - 1);
imaxX = Mathf.Clamp(imaxX, 0, width - 1);
imaxY = Mathf.Clamp(imaxY, 0, height - 1);
imaxZ = Mathf.Clamp(imaxZ, 0, depth - 1);

// 三角形のAABBの中でボクセルとの交差判定を行う
for(int x = iminX; x <= imaxX; x++) {
    for(int y = iminY; y <= imaxY; y++) {
        for(int z = iminZ; z <= imaxZ; z++) {
            if(Intersects(tri, boxes[x, y, z])) {
                ...
            }
        }
    }
}
//}

三角形とボクセルとの交差判定はIntersects(Triangle, Bounds)関数で行います。

//emlist[CPUVoxelizer.cs]{
public static bool Intersects(Triangle tri, Bounds aabb)
{
    ...
}
//}

この関数内では、前述の13通りの軸について交差判定を行っていきますが、
AABBの3つの法線が既知である（XYZ軸に沿った辺を持つので、単にX軸（1, 0, 0）、Y軸（0, 1, 0）、Z軸（0, 0, 1）の法線を持つ）ことを利用したり、
AABBの中心が原点（0, 0, 0）に来るように三角形とAABBの座標を平行移動させることで、
交差判定を最適化しています。

//emlist[CPUVoxelizer.cs]{

// AABBの中心座標と各辺の半分の長さ(extents)を取得
Vector3 center = aabb.center, extents = aabb.max - center;

// AABBの中心が原点(0, 0, 0)に来るように三角形の座標を平行移動
Vector3 v0 = tri.a - center,
        v1 = tri.b - center,
        v2 = tri.c - center;

// 三角形の三辺を表すベクトルを取得
Vector3 f0 = v1 - v0,
        f1 = v2 - v1,
        f2 = v0 - v2;

//}

まずは、三角形の3つの辺とAABBの直交する3つの辺同士の組み合わせから得られる9通りのクロス積を軸とした交差判定を行っていきますが、
AABBの3つの辺の方向がXYZ軸に平行であることを利用し、クロス積を得るための計算を省くことができます。

//emlist[CPUVoxelizer.cs]{

// AABBの辺はそれぞれ方向ベクトルx(1, 0, 0)、y(0, 1, 0)、z(0, 0, 1)であるので、
// 計算を行わずに9通りのクロス積を得ることができる
Vector3 
    a00 = new Vector3(0, -f0.z, f0.y), // X軸とf0とのクロス積
    a01 = new Vector3(0, -f1.z, f1.y), // Xとf1
    a02 = new Vector3(0, -f2.z, f2.y), // Xとf2
    a10 = new Vector3(f0.z, 0, -f0.x), // Yとf0
    a11 = new Vector3(f1.z, 0, -f1.x), // Yとf1
    a12 = new Vector3(f2.z, 0, -f2.x), // Yとf2
    a20 = new Vector3(-f0.y, f0.x, 0), // Zとf0
    a21 = new Vector3(-f1.y, f1.x, 0), // Zとf1
    a22 = new Vector3(-f2.y, f2.x, 0); // Zとf2

// 9つの軸について交差判定を行う（後述）
// （どれか一つでも交差しない軸があれば、三角形とAABBは交差していないのでfalseを返す）
if (
    !Intersects(v0, v1, v2, extents, a00) ||
    !Intersects(v0, v1, v2, extents, a01) ||
    !Intersects(v0, v1, v2, extents, a02) ||
    !Intersects(v0, v1, v2, extents, a10) ||
    !Intersects(v0, v1, v2, extents, a11) ||
    !Intersects(v0, v1, v2, extents, a12) ||
    !Intersects(v0, v1, v2, extents, a20) ||
    !Intersects(v0, v1, v2, extents, a21) ||
    !Intersects(v0, v1, v2, extents, a22)
)
{
    return false;
}

//}

これらの軸上で三角形とAABBを射影して交差を判定するのが以下の関数です。

//emlist[CPUVoxelizer.cs]{
protected static bool Intersects(
    Vector3 v0, 
    Vector3 v1, 
    Vector3 v2, 
    Vector3 extents, 
    Vector3 axis
)
{
    ...
}
//}

ここで注意したいのが、
AABBの中心を原点に持ってくることによる最適化を行っていることです。
AABBの各頂点を全て軸に射影する必要はなく、
AABBのXYZ軸について最大値を持つ頂点、つまり各辺の半分の長さ（extents）を射影するだけで軸上の区間を得ることができます。

extentsを射影して得られた値rはAABBの射影軸上の区間[-r, r]を表すので、
AABBについては射影の計算が一回で済む、ということです。

//emlist[CPUVoxelizer.cs]{
// 三角形の頂点を軸上に射影する
float p0 = Vector3.Dot(v0, axis);
float p1 = Vector3.Dot(v1, axis);
float p2 = Vector3.Dot(v2, axis);

// AABBのXYZ軸について最大値を持つ頂点（extents）を軸上に射影し、値rを得る
// AABBの区間は[-r, r]であるので、AABBについては全頂点を射影する必要はない
float r = 
    extents.x * Mathf.Abs(axis.x) + 
    extents.y * Mathf.Abs(axis.y) + 
    extents.z * Mathf.Abs(axis.z);

// 三角形の射影区間
float minP = Mathf.Min(p0, p1, p2);
float maxP = Mathf.Max(p0, p1, p2);

// 三角形の区間とAABBの区間が重なっているかの判別
return !((maxP < -r) || (r < minP));

//}

9通りのクロス積を軸とした判別の次は、
AABBの3つの法線を軸として判別を行います。

AABBの法線がXYZ軸に平行であるという特性を利用し、
AABBの中心を原点に持ってくるよう座標値を平行移動しているので、
単に三角形の各頂点のXYZ成分についての最小値・最大値とextentsを比較するだけで交差判定が行えます。

//emlist[CPUVoxelizer.cs]{
// X軸
if (
    Mathf.Max(v0.x, v1.x, v2.x) < -extents.x || 
    Mathf.Min(v0.x, v1.x, v2.x) > extents.x
)
{
    return false;
}

// Y軸
if (
    Mathf.Max(v0.y, v1.y, v2.y) < -extents.y || 
    Mathf.Min(v0.y, v1.y, v2.y) > extents.y
)
{
    return false;
}

// Z軸
if (
    Mathf.Max(v0.z, v1.z, v2.z) < -extents.z || 
    Mathf.Min(v0.z, v1.z, v2.z) > extents.z
)
{
    return false;
}
//}

最後は三角形の法線についてですが、
三角形の法線を持つPlaneとAABBとの交差について判定を行っています。

//emlist[CPUVoxelizer.cs]{
var normal = Vector3.Cross(f1, f0).normalized;
var pl = new Plane(normal, Vector3.Dot(normal, tri.a));
return Intersects(pl, aabb);
//}

Intersects(Plane, Bounds)関数でPlaneとAABBとの交差を判定します。

//emlist[CPUVoxelizer.cs]{
public static bool Intersects(Plane pl, Bounds aabb)
{
    Vector3 center = aabb.center;
    var extents = aabb.max - center;

    // Planeのnormal上にextentsを射影
    var r = 
        extents.x * Mathf.Abs(pl.normal.x) + 
        extents.y * Mathf.Abs(pl.normal.y) + 
        extents.z * Mathf.Abs(pl.normal.z);

    // PlaneとAABBの中心との距離を計算
    var s = Vector3.Dot(pl.normal, center) - pl.distance;

    // sが[-r, r]の範囲にあるかどうか判定
    return Mathf.Abs(s) <= r;
}
//}

==== 交差したボクセルを配列データに書き込む

一つの三角形について、交差するボクセルを判定できると、
ボクセルデータのfillフラグを立て、その三角形が決められた方向から見て前面か背面かを示すfrontフラグを設定します。
（frontフラグについては後述）

ボクセルによっては前面を向いている三角形と背面を向いている三角形両方と交差している場合がありますが、
その場合、frontフラグは背面を優先するようにします。

//emlist[CPUVoxelizer.cs]{
if(Intersects(tri, boxes[x, y, z])) {
    // 交差した(x, y, z)にあるボクセルを取得する
    var voxel = volume[x, y, z];

    // ボクセルの位置を設定する
    voxel.position = boxes[x, y, z].center;

    if(voxel.fill & 1 == 0) {
        // ボクセルがまだ埋まっていない場合
        // ボクセルと交差した三角形が前面かどうかのフラグを立てる
        voxel.front = front;
    } else {
        // ボクセルが既にほかの三角形で埋められている場合
        // 背面のフラグを優先する
        voxel.front = voxel.front & front;
    }

    // ボクセルを埋めるフラグを立てる
    voxel.fill = 1;
    volume[x, y, z] = voxel;
}
//}

frontフラグは後述の「メッシュの中身を埋める処理」に必要で、
「中身を埋めていく方向」から見て前面か背面かを設定します。

サンプルコードでは、forward(0, 0, 1)方向にメッシュの中身を埋めていくので、
三角形がforward(0, 0, 1)から見て前面かどうかを判定します。

3角形の法線とボクセルを埋める方向との内積が0以下であれば、
3角形はその方向から見て前面であることがわかります。

//emlist[CPUVoxelizer.cs]{
public class Triangle {
    public Vector3 a, b, c;     // 3角形を構成する3点
    public bool frontFacing;    // 3角形がボクセルを埋める方向から見て表面かどうかのフラグ
    public Bounds bounds;       // 3角形のAABB

    public Triangle (Vector3 a, Vector3 b, Vector3 c, Vector3 dir) {
        this.a = a;
        this.b = b;
        this.c = c;

        // 3角形がボクセルを埋める方向から見て前面かどうかを判定する
        var normal = Vector3.Cross(b - a, c - a);
        this.frontFacing = (Vector3.Dot(normal, dir) <= 0f);

        ...
    }
}
//}

=== メッシュの表面を表すボクセルデータから、メッシュ内部に位置するボクセルを埋める

メッシュ表面に位置するボクセルデータが計算できたので、次はその内部を埋めていきます。

//image[Nakamura/VoxelFill][メッシュ表面に位置するボクセルデータを生成した後の状態]

==== ボクセルを埋める流れ

ボクセルを埋める方向からみて前面を向いているボクセルを探索します。

以下の図のように空のボクセルは素通りします。

//image[Nakamura/VoxelFill0][ボクセルを埋める方向からみて前面を向いているボクセルを探索する　空のボクセルは素通りする（矢印がボクセルを埋める方向で、枠が探索中のボクセル位置を表す）]

前面を向いているボクセルを見つけたら、
前面を向いているボクセルの中を進んでいきます。

//image[Nakamura/VoxelFill1][前面を向いているボクセルを見つけた状態（メッシュ表面から出ている線はメッシュの法線であり、図ではメッシュの法線とボクセルを埋める方向が向かいあっているので、枠の位置のボクセルは前面に位置していることがわかる）]

//image[Nakamura/VoxelFill2][前面を向いているボクセルの中を進む]

前面を向いているボクセルの中を抜けると、メッシュ内部に到達します。

//image[Nakamura/VoxelFill3][前面を向いているボクセルの中を抜け、メッシュ内部に到達した状態]

メッシュ内部の中を進んでいき、
到達したボクセルを埋めていきます。

//image[Nakamura/VoxelFill4][メッシュ内部を埋めていくように、到達したボクセルを埋めていく]

そして、ボクセルを埋める方向からみて背面を向いているボクセルに到達すると、
メッシュの内部を埋めきったことがわかります。
背面を向いたボクセルの中を進んでいき、
またメッシュの外に到達すると、また前面を向いているボクセルを探索しはじめます。

//image[Nakamura/VoxelFill5][ボクセルを埋める方向から背面を向いているボクセルの中を進んでいき、またメッシュの外に到達する]

==== ボクセルを埋める実装

前項で決めたようにforward(0, 0, 1)方向に向かって内部を埋めていくので、
3次元ボクセル配列ではz方向に向かって内部を埋めていくことになります。

z方向の一番手前側、volume[x, y, 0]からはじめてvolume[x, y, depth - 1]まで中身を埋める処理を進めます。

//emlist[CPUVoxelizer.cs]{

// メッシュの内部を埋める
for(int x = 0; x < width; x++)
{
    for(int y = 0; y < height; y++)
    {
        // z方向の手前側から奥に向かってメッシュ内部を埋めていく
        for(int z = 0; z < depth; z++)
        {
            ...
        }
    }
}
//}

既にボクセルデータに書き込まれてあるfrontフラグ（z方向に向かって前面か背面か）を基に、
前述のボクセルを埋める流れに沿って処理を進めます。

//emlist[CPUVoxelizer.cs]{

...
// z方向の手前側から奥に向かってメッシュ内部を埋めていく
for(int z = 0; z < depth; z++)
{
    // (x, y, z)が空の場合は無視
    if (volume[x, y, z].IsEmpty()) continue;

    // 前面に位置するボクセルを進む
    int ifront = z;
    for(; ifront < depth && volume[x, y, ifront].IsFrontFace(); ifront++) {}

    // 最後までいけば終わり
    if(ifront >= depth) break;

    // 背面に位置するボクセルを探す
    int iback = ifront;

    // メッシュ内部を進んでいく
    for (; iback < depth && volume[x, y, iback].IsEmpty(); iback++) {}

    // 最後までいけば終わり
    if (iback >= depth) break;

    // (x, y, iback)が背面かどうかを判定
    if(volume[x, y, iback].IsBackFace()) {
        // 背面に位置するボクセルを進む
        for (; iback < depth && volume[x, y, iback].IsBackFace(); iback++) {}
    }

    // (x, y, ifront)から(x, y, iback)の位置までボクセルを埋める
    for(int z2 = ifront; z2 < iback; z2++)
    {
        var p = boxes[x, y, z2].center;
        var voxel = volume[x, y, z2];
        voxel.position = p;
        voxel.fill = 1;
        volume[x, y, z2] = voxel;
    }

    // 処理し終えた(x, y, iback)までループを進める
    z = iback;
}
//}

ここまででメッシュの中身を充填したボクセルデータを得ることができました。

処理し終えた3次元のボクセルデータの中には、空のボクセルが含まれているため、
CPUVoxelizer.Voxelizeではメッシュの表面と充填された中身を構成するボクセルのみを返すようにしています。

//emlist[CPUVoxelizer.cs]{
// 空でないボクセルを取得する
voxels = new List<Voxel_t>();
for(int x = 0; x < width; x++) {
    for(int y = 0; y < height; y++) {
        for(int z = 0; z < depth; z++) {
            if(!volume[x, y, z].IsEmpty())
            {
                voxels.Add(volume[x, y, z]);
            }
        }
    }
}
//}

CPUVoxelizerTest.csで、CPUVoxelizerで得たボクセルデータを用いてメッシュを構築し、ボクセルを可視化しています。

//image[Nakamura/CPUVoxelizerTest][CPUVoxelizer.Voxelizeで得たボクセルデータをMeshにして可視化したデモ（CPUVoxelizerTest.scene）]

== ボクセルのメッシュ表現

ボクセルデータの配列Voxel_t[]と、
一つのボクセルの単位長さの情報を基にメッシュを構築する処理をVoxelMeshクラスに記述しています。

前節のCPUVoxelizerTest.csではこのクラスを用いてボクセルメッシュの生成を行っています。

//emlist[VoxelMesh.cs]{
public class VoxelMesh {

    public static Mesh Build (Voxel_t[] voxels, float size)
    {
        var hsize = size * 0.5f;
        var forward = Vector3.forward * hsize;
        var back = -forward;
        var up = Vector3.up * hsize;
        var down = -up;
        var right = Vector3.right * hsize;
        var left = -right;

        var vertices = new List<Vector3>();
        var normals = new List<Vector3>();
        var triangles = new List<int>();

        for(int i = 0, n = voxels.Length; i < n; i++)
        {
            if(voxel[i].fill == 0) continue;

            var p = voxels[i].position;

            // 一つのボクセルを表現するCubeを構成する8隅の頂点
            var corners = new Vector3[8] {
                p + forward + left + up,
                p + back + left + up,
                p + back + right + up,
                p + forward + right + up,

                p + forward + left + down,
                p + back + left + down,
                p + back + right + down,
                p + forward + right + down,
            };

            // Cubeを構成する6面を構築する

            // up
            AddTriangle(
                corners[0], corners[3], corners[1], 
                up, vertices, normals, triangles
            );
            AddTriangle(
                corners[2], corners[1], corners[3], 
                up, vertices, normals, triangles
            );

            // down
            AddTriangle(
                corners[4], corners[5], corners[7], 
                down, vertices, normals, triangles
            );
            AddTriangle(
                corners[6], corners[7], corners[5], 
                down, vertices, normals, triangles
            );

            // right
            AddTriangle(
                corners[7], corners[6], corners[3], 
                right, vertices, normals, triangles
            );
            AddTriangle(
                corners[2], corners[3], corners[6], 
                right, vertices, normals, triangles
            );

            // left
            AddTriangle(
                corners[5], corners[4], corners[1],
                left, vertices, normals, triangles
            );
            AddTriangle(
                corners[0], corners[1], corners[4],
                left, vertices, normals, triangles
            );

            // forward
            AddTriangle(
                corners[4], corners[7], corners[0],
                forward, vertices, normals, triangles
            );
            AddTriangle(
                corners[3], corners[0], corners[7],
                forward, vertices, normals, triangles
            );

            // back
            AddTriangle(
                corners[6], corners[5], corners[2],
                forward, vertices, normals, triangles
            );
            AddTriangle(
                corners[1], corners[2], corners[5],
                forward, vertices, normals, triangles
            );
        }

        var mesh = new Mesh();
        mesh.SetVertices(vertices);

        // 頂点数が16bitでサポートできる数を超えていたら32bit index formatを適用する
        mesh.indexFormat = 
            (vertices.Count <= 65535) 
            ? IndexFormat.UInt16 : IndexFormat.UInt32;
        mesh.SetNormals(normals);
        mesh.SetIndices(triangles.ToArray(), MeshTopology.Triangles, 0);
        mesh.RecalculateBounds();
        return mesh;
    }
}
//}

== GPUでの実装

ここからはGPUを用いてCPUVoxelizerで実装したボクセル化をより高速に実行する方法について解説します。

CPUVoxelizerで実装したボクセル化のアルゴリズムは、
XY平面上でボクセルの単位長さで区切られた格子空間上の各座標ごとに並列化することができます。

//image[Nakamura/Parallelization][XY平面上でボクセルの単位長さで区切られた格子空間　ボクセル化はこの各格子ごとに並列化できるのでGPU実装が可能]

並列化できるそれぞれの処理をGPUスレッドに割り振れば、
GPUの高速な並列計算の恩恵により、高速に処理が実行できます。

GPUでのボクセル化の実装はGPUVoxelizer.csとVoxelizer.computeに記述しています。

（本節から登場する、UnityでGPGPUプログラミングをする上で欠かせないComputeShaderの基本についてはUnity Graphics Programming vol.1「ComputeShader入門」で解説しています）

GPUでのボクセル化は、GPUVoxelizerクラスのstatic関数
//emlist[GPUVoxelizer.cs]{
public class GPUVoxelizer
{
    public static GPUVoxelData Voxelize (
        ComputeShader voxelizer, 
        Mesh mesh, 
        int resolution
    ) {
    ...
    }
}
//}
をコールすることで実行します。
引数にVoxelizer.compute、ボクセル化したいメッシュ、解像度を指定して実行すると、
ボクセルデータを示すGPUVoxelDataを返します。

=== GPUでのボクセル生成に必要なデータのセットアップ

ボクセル化の大まかな流れ(1) ~ (3)と同じようにして、
ボクセル生成に必要なデータのセットアップを行います。

//emlist[GPUVoxelizer.cs]{
public static GPUVoxelData Voxelize (
    ComputeShader voxelizer, 
    Mesh mesh, 
    int resolution
) {
    // CPUVoxelizer.Voxelizeと同じ処理 -------
    mesh.RecalculateBounds();
    var bounds = mesh.bounds;

    float maxLength = Mathf.Max(
        bounds.size.x, 
        Mathf.Max(bounds.size.y, bounds.size.z)
    );
    var unit = maxLength / resolution;

    var hunit = unit * 0.5f;

    var start = bounds.min - new Vector3(hunit, hunit, hunit);
    var end = bounds.max + new Vector3(hunit, hunit, hunit);
    var size = end - start;

    int width = Mathf.CeilToInt(size.x / unit);
    int height = Mathf.CeilToInt(size.y / unit);
    int depth = Mathf.CeilToInt(size.z / unit);
    // ------- ここまでCPUVoxelizer.Voxelizeと同じ
    ...
}
//}

Voxel_tの配列は、GPU上で扱えるようにするためにComputeBufferとして定義します。
ここで注意したいのが、CPU実装だと3次元配列として生成したVoxel_t配列を1次元配列として定義していることです。

これは、GPUでは多次元配列を扱うのが困難なためで、
1次元配列として定義しておき、ComputeShader内では3次元上の位置(x, y, z)から1次元配列上のindexを取得することで、
1次元配列を3次元配列のように処理しています。

//emlist[GPUVoxelizer.cs]{
// Voxel_t配列を表すComputeBufferを生成
var voxelBuffer = new ComputeBuffer(
    width * height * depth, 
    Marshal.SizeOf(typeof(Voxel_t))
);
var voxels = new Voxel_t[voxelBuffer.count];
voxelBuffer.SetData(voxels); // 初期化
//}

これらセットアップしたデータをGPU側に転送します。

//emlist[GPUVoxelizer.cs]{
// ボクセルデータをGPU側に転送
voxelizer.SetVector("_Start", start);
voxelizer.SetVector("_End", end);
voxelizer.SetVector("_Size", size);

voxelizer.SetFloat("_Unit", unit);
voxelizer.SetFloat("_InvUnit", 1f / unit);
voxelizer.SetFloat("_HalfUnit", hunit);
voxelizer.SetInt("_Width", width);
voxelizer.SetInt("_Height", height);
voxelizer.SetInt("_Depth", depth);
//}

メッシュを構成する三角形とボクセル同士の交差判定を行うため、
メッシュを表すComputeBufferを生成します。

//emlist[GPUVoxelizer.cs]{
// メッシュの頂点配列を表すComputeBufferを生成
var vertices = mesh.vertices;
var vertBuffer = new ComputeBuffer(
    vertices.Length, 
    Marshal.SizeOf(typeof(Vector3))
);
vertBuffer.SetData(vertices);

// メッシュの三角形配列を表すComputeBufferを生成
var triangles = mesh.triangles;
var triBuffer = new ComputeBuffer(
    triangles.Length, 
    Marshal.SizeOf(typeof(int))
);
triBuffer.SetData(triangles);
//}

=== GPUでメッシュの表面に位置するボクセルを生成する

GPUでメッシュの表面に位置するボクセルを生成する処理では、
前面を向いている三角形と交差しているボクセルを生成した後で、
背面を向いている三角形と交差しているボクセルを生成します。

これは、同じ位置のボクセルに対して複数の三角形が交差している場合に、
ボクセルに書き込まれるfrontフラグの値が一意に定まらない恐れがあるためです。

GPUの並列計算で気をつけないといけないのは、
同じデータに複数のスレッドが同時にアクセスしてしまうことによる、結果の不定性です。

この表面を生成する処理においては、frontフラグの値が背面（false）であることを優先し、
前面→背面という順でボクセル生成を実行することで結果の不定性を防いでいます。

前面を向いている三角形と交差しているボクセルを生成するGPUカーネルSurfaceFrontに、
先ほど生成したメッシュデータを転送します。

//emlist[GPUVoxelizer.cs]{
// GPUカーネルSurfaceFrontにメッシュデータを転送
var surfaceFrontKer = new Kernel(voxelizer, "SurfaceFront");
voxelizer.SetBuffer(surfaceFrontKer.Index, "_VoxelBuffer", voxelBuffer);
voxelizer.SetBuffer(surfaceFrontKer.Index, "_VertBuffer", vertBuffer);
voxelizer.SetBuffer(surfaceFrontKer.Index, "_TriBuffer", triBuffer);

// メッシュを構成する三角形の数を設定
var triangleCount = triBuffer.count / 3; // (三角形を構成する頂点indexの数 / 3)が三角形の数
voxelizer.SetInt("_TriangleCount", triangleCount);
//}

この処理はメッシュを構成する三角形ごとに並列に実行します。
全ての三角形が処理されるように、カーネルのスレッドグループを(三角形の数triangleCount / カーネルのスレッド数 + 1, 1, 1)に設定し、
カーネルを実行します。

//emlist[GPUVoxelizer.cs]{
// 前面を向いている三角形と交差するボクセルを構築
voxelizer.Dispatch(
    surfaceFrontKer.Index, 
    triangleCount / (int)surfaceFrontKer.ThreadX + 1, 
    (int)surfaceFrontKer.ThreadY, 
    (int)surfaceFrontKer.ThreadZ
);
//}

SurfaceFrontカーネルは前面を向いている三角形のみを処理するため、
三角形の前面背面をチェックし、背面である場合はそのまま処理を終了するようにreturnし、
前面である場合はメッシュ表面を構築するsurface関数を実行しています。

//emlist[Voxelizer.compute]{
[numthreads(8, 1, 1)]
void SurfaceFront (uint3 id : SV_DispatchThreadID)
{
    // 三角形の数を超えているとreturn
    int idx = (int)id.x;
    if(idx >= _TriangleCount) return;

    // 三角形の頂点位置と前面背面フラグを取得
    float3 va, vb, vc;
    bool front;
    get_triangle(idx, va, vb, vc, front);

    // 背面である場合はreturn
    if (!front) return;

    // メッシュ表面を構築
    surface(va, vb, vc, front);
}
//}

get_triangle関数は、
CPUからGPU側に渡されたメッシュデータ（三角形を構成する頂点indexを表す_TriBufferと頂点を表す_VertBuffer）に基づいて、
三角形の頂点位置と前面背面フラグを取得します。

//emlist[Voxelizer.compute]{
void get_triangle(
    int idx, 
    out float3 va, out float3 vb, out float3 vc, 
    out bool front
)
{
    int ia = _TriBuffer[idx * 3];
    int ib = _TriBuffer[idx * 3 + 1];
    int ic = _TriBuffer[idx * 3 + 2];

    va = _VertBuffer[ia];
    vb = _VertBuffer[ib];
    vc = _VertBuffer[ic];

    // 三角形がforward(0, 0, 1)方向から見て前面か背面かを判断
    float3 normal = cross((vb - va), (vc - vb));
    front = dot(normal, float3(0, 0, 1)) < 0;
}
//}

ボクセルと三角形との交差判定を行い、その結果をボクセルデータに書き込むsurface関数は、
1次元配列として生成したボクセルデータのindexを取得する手間があれど、
その処理の内容はCPUVoxelizer上に実装したものとほぼ同じものになります。

//emlist[Voxelizer.compute]{
void surface (float3 va, float3 vb, float3 vc, bool front)
{
    // 三角形のAABBを計算
    float3 tbmin = min(min(va, vb), vc);
    float3 tbmax = max(max(va, vb), vc);

    float3 bmin = tbmin - _Start;
    float3 bmax = tbmax - _Start;
    int iminX = round(bmin.x / _Unit);
    int iminY = round(bmin.y / _Unit);
    int iminZ = round(bmin.z / _Unit);
    int imaxX = round(bmax.x / _Unit);
    int imaxY = round(bmax.y / _Unit);
    int imaxZ = round(bmax.z / _Unit);
    iminX = clamp(iminX, 0, _Width - 1);
    iminY = clamp(iminY, 0, _Height - 1);
    iminZ = clamp(iminZ, 0, _Depth - 1);
    imaxX = clamp(imaxX, 0, _Width - 1);
    imaxY = clamp(imaxY, 0, _Height - 1);
    imaxZ = clamp(imaxZ, 0, _Depth - 1);

    // 三角形のAABBの中でボクセルとの交差判定を行う
    for(int x = iminX; x <= imaxX; x++) {
        for(int y = iminY; y <= imaxY; y++) {
            for(int z = iminZ; z <= imaxZ; z++) {
                // (x, y, z)に位置するボクセルのAABBを生成
                float3 center = float3(x, y, z) * _Unit + _Start;
                AABB aabb;
                aabb.min = center - _HalfUnit;
                aabb.center = center;
                aabb.max = center + _HalfUnit;
                if(intersects_tri_aabb(va, vb, vc, aabb))
                {
                    // (x, y, z)の位置から1次元のボクセル配列のindexを取得
                    uint vid = get_voxel_index(x, y, z);
                    Voxel voxel = _VoxelBuffer[vid];
                    voxel.position = get_voxel_position(x, y, z);
                    voxel.front = front;
                    voxel.fill = true;
                    _VoxelBuffer[vid] = voxel;
                }
            }
        }
    }
}
//}

これで前面を向いている三角形についてボクセルが生成できたので、
次は背面を向いている三角形について処理していきます。

背面を向いている三角形と交差するボクセルを生成するGPUカーネルSurfaceBackに先程と同じようにメッシュデータを転送し、
実行します。

//emlist[GPUVoxelizer.cs]{
var surfaceBackKer = new Kernel(voxelizer, "SurfaceBack");
voxelizer.SetBuffer(surfaceBackKer.Index, "_VoxelBuffer", voxelBuffer);
voxelizer.SetBuffer(surfaceBackKer.Index, "_VertBuffer", vertBuffer);
voxelizer.SetBuffer(surfaceBackKer.Index, "_TriBuffer", triBuffer);
voxelizer.Dispatch(
    surfaceBackKer.Index, 
    triangleCount / (int)surfaceBackKer.ThreadX + 1, 
    (int)surfaceBackKer.ThreadY, 
    (int)surfaceBackKer.ThreadZ
);
//}

SurfaceBackの処理は、三角形が前面を向いている場合にreturnを返す以外はSurfaceFrontと同じです。
SurfaceFrontの後にSurfaceBackを実行することによって、
もし前面を向いている三角形と背面を向いている三角形両方と交差しているボクセルが存在していても、
ボクセルのfrontフラグがSurfaceBackによって上書きされる形になり、
背面を向いていることが優先されるようになります。

//emlist[Voxelizer.compute]{
[numthreads(8, 1, 1)]
void SurfaceBack (uint3 id : SV_DispatchThreadID)
{
    int idx = (int)id.x;
    if(idx >= _TriangleCount) return;

    float3 va, vb, vc;
    bool front;
    get_triangle(idx, va, vb, vc, front);

    // 前面である場合はreturn
    if (front) return;

    surface(va, vb, vc, front);
}
//}

=== GPUでメッシュの表面を表すボクセルデータから、メッシュ内部に位置するボクセルを埋める

メッシュの内部を埋める処理はVolumeカーネルで行います。

VolumeカーネルはXY平面上でボクセルの単位長さで区切られた格子空間上の各座標ごとにスレッドを用意して実行します。
つまり、CPU実装だとXY座標について二重ループを実行していたところをGPUで並列化し、高速化しているということになります。

//emlist[GPUVoxelizer.cs]{
// Volumeカーネルにボクセルデータを転送
var volumeKer = new Kernel(voxelizer, "Volume");
voxelizer.SetBuffer(volumeKer.Index, "_VoxelBuffer", voxelBuffer);

// メッシュ内部を埋める
voxelizer.Dispatch(
    volumeKer.Index, 
    width / (int)volumeKer.ThreadX + 1, 
    height / (int)volumeKer.ThreadY + 1, 
    (int)volumeKer.ThreadZ
);
//}

Volumeカーネルの実装はGPUVoxelizerに実装したものとほぼ同じものになります。

//emlist[Voxelizer.compute]{
[numthreads(8, 8, 1)]
void Volume (uint3 id : SV_DispatchThreadID)
{
    int x = (int)id.x;
    int y = (int)id.y;
    if(x >= _Width) return;
    if(y >= _Height) return;

    for (int z = 0; z < _Depth; z++)
    {
        Voxel voxel = _VoxelBuffer[get_voxel_index(x, y, z)];
        // CPUVoxelizer.Voxelize内の処理とほぼ同じ処理が続く
        ...
    }
}
//}

このようにしてボクセルデータが得られると、
不要になったメッシュデータを破棄し、
ボクセルのビジュアル表現をつくる際に必要なデータを持ったGPUVoxelDataを生成します。

//emlist[GPUVoxelizer.cs]{
// 不要になったメッシュデータを破棄
vertBuffer.Release();
triBuffer.Release();

return new GPUVoxelData(voxelBuffer, width, height, depth, unit);
//}

これでGPU実装によるボクセル化が完了しました。
GPUVoxelizerTest.csで実際にGPUVoxelDataを用いてボクセルデータを可視化しています。

== CPU実装とGPU実装の速度差

テスト用のSceneではPlay時にVoxelizerを実行しているので、CPU実装とGPU実装の速度差がわかりにくいですが、
GPU実装でかなりの高速化を実現しています。

実行環境と、ボクセル化する対象のメッシュのポリゴン数、ボクセル化の解像度にパフォーマンスは大きく依存しますが、

 * 実行環境 OS: Windows10、CPU: Core i7、メモリ: 32GB、GPU: GeForce GTX 980
 * 頂点数 5319、三角形数 9761のメッシュ
 * ボクセル化の解像度 256

という条件では、GPU実装はCPU実装の50倍以上高速に動作しています。

== 応用例

GPU実装のParticleSystemを利用した応用例（GPUVoxelParticleSystem）を紹介します。

GPUVoxelParticleSystemはGPUVoxelizerから得られたボクセルデータを表すComputeBufferを、
ComputeShaderでのパーティクルの位置計算に利用します。

 1. GPUVoxelizerでアニメーションモデルを毎フレームボクセル化
 2. GPUVoxelDataが持つComputeBufferを、パーティクルの位置計算を行うComputeShaderに渡す
 3. パーティクルをGPUインスタンシングで描画

という流れでエフェクトを作成しています。

//image[Nakamura/GPUVoxelParticleSystem3][GPU実装のParticleSystemを利用した応用例（GPUVoxelParticleSystem）]

大量のパーティクルをボクセルの位置から出現させることで、
パーティクルで構成されるアニメーションモデルのようなビジュアルを実現しています。

アニメーションモデルに対してフレーム毎にボクセル化を施せるのは、
GPU実装による高速化があってこそで、
リアルタイムで利用できるビジュアル表現の幅を広げるためにも、
こうしたGPUでの高速化は欠かせないものになっています。

== まとめ

本章では、メッシュモデルのボクセル化を行うアルゴリズムをCPU実装を例に紹介し、
GPU実装によってボクセル化を高速化するところまで行いました。

三角形とボクセルとの交差判定を用いてボクセルを生成するアプローチをとりましたが、
平行投影によってモデルをXYZ方向から3Dテクスチャにレンダリングしていくことでボクセルデータを構築する方法もあります。

本章で紹介した手法だと、ボクセル化後のモデルにどうテクスチャを貼るかといった点に課題がありますが、
3Dテクスチャにモデルをレンダリングする手法であれば、ボクセルへの色付けはより手軽かつ正確に実現できるかもしれません。

== 参考

 * http://blog.wolfire.com/2009/11/Triangle-mesh-voxelization
 * http://www.dyn4j.org/2010/01/sat/
 * https://gdbooks.gitbooks.io/3dcollisions/content/Chapter4/aabb-triangle.html
 * ゲームエンジン・アーキテクチャ第2版 第12章
 * https://developer.nvidia.com/content/basics-gpu-voxelization


