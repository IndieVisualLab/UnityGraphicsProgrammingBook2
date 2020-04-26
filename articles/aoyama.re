= Space Filling

== はじめに
本章では@<b>{Space filling問題}@<fn>{sp}にフォーカスし、それを解決する手法のひとつである@<b>{アポロニウスのギャスケット}について解説します。

なお、本章ではアポロニウスのギャスケットのアルゴリズム解説がメインとなるため、グラフィックプログラミングの話からは少し逸脱しています。

== Space filling問題
Space filling問題とは、ひとつの閉じた平面の内部を重なることなく、ある形状で可能な限り埋め尽くす手法を発見するという問題です。
この問題は特に幾何学分野や組合せ最適化分野では古くから研究がされてきた領域です。
どのような形状の平面に、どうのような形状で埋め尽くすかという組み合わせは無数に存在するので、それぞれの組み合わせに対してさまざまな手法が提案されています。

いくつか例を挙げると

 * 矩形パッキング@<fn>{rectangle}　：O-Tree法
 * 多角形パッキング@<fn>{polygon}：Bottom-Left法
 * 円パッキング@<fn>{circle}　　：アポロニウスのギャスケット
 * 三角形パッキング@<fn>{triangle}：シェルピンスキーのギャスケット

//footnote[sp][他にも「平面充填」「パッキング問題」「詰め込み問題」などという呼ばれ方をされています]
//footnote[rectangle][矩形パッキング @<m>{\cdots} 矩形状の平面内を矩形で埋める]
//footnote[polygon][多角形パッキング @<m>{\cdots} 矩形状の平面内を多角形で埋める]
//footnote[circle][円パッキング @<m>{\cdots} 円状の平面内を円で埋める]
//footnote[triangle][三角形パッキング @<m>{\cdots} 三角形状の平面内を三角形で埋める]

などがあり、他にも多くの手法が存在します。
本章では上記のうち、アポロニウスのギャスケットについて解説を行います。

なお、Space filling問題はNP困難であることが知られており、上記のどのアルゴリズムを用いても、平面内を常に100％埋めるのは現状では難しいとされています。
アポロニウスのギャスケットに関しても同様であり、円の内部を円で完全に埋めきることはできません。

== アポロニウスのギャスケット
アポロニウスのギャスケットとは、互いに接する3つの円から生成されるフラクタル図形の一種です。
これは最初期のフラクタル図形の一種であり、Space filling問題を解決するために提案されたアルゴリズムなどではなく、平面幾何学の研究成果の一つがSpace filling問題の一つの解に成り得たという話です。
この名前は、紀元前のギリシャ人数学者であるペルガのアポロニウスにちなんで付けられています。

まず、互いに接する3つの円をそれぞれC1,C2,C3とすると、アポロニウスはC1,C2,C3全てと接する交差しない2つの円C4,C5が存在することを発見しました。
このC4,C5は、C1,C2,C3に対する@<b>{アポロニウスの円（詳細は後述）}となります。
//image[aoyama/circle1][C1,C2,C3とそれに接する2つの円C4,C5][scale=0.8]

ここで、C1,C2に対してC4を考えると、C1,C2,C4について新たな2つのアポロニウスの円を得ることができます。
この2つの円のうち、一方はC3となりますが、他方は新しい円C6となります。

これを（C1,C2,C5）,（C2, C3, C4）,（C1, C3, C4）のように全ての組み合わせについてアポロニウスの円を考えると、それぞれについて最低1つの新しい円を得ることができます。
これを無限に繰り返していくことにより、それぞれが接し合う円の集合が作り出されます。
この円の集合が、アポロニウスのギャスケットです。
//image[aoyama/apollonian_gasket][アポロニウスのギャスケット][scale=0.9]
https://upload.wikimedia.org/wikipedia/commons/e/e6/Apollonian_gasket.svg

//note[アポロニウスの円]{
    2つの定点A,Bを取り点Pを@<m>{AP : BP = 一定}となるように取ったときの点Pの軌跡のことです。
    ただ、これとは別にアポロニウスの問題に対する解を指して、アポロニウスの円と呼ばれることもあり、アポロニウスのギャスケットにおいてはこちらの意味合いの方が強いです。
//}
//note[アポロニウスの問題]{
    ユークリッド平面幾何学において、与えられた3つの円に接する4つ目の円を描くという問題です。
    この4つ目の円は最大で8つの解が存在するとされており、その内2つの解は常に3円と外接し、2つの円は常に3円と内接するとされています。

    ちなみに、条件として与えられる3つの円は接し合っている必要はなく、あくまでその3つの円に接する4つ目の円を描くというのが問題です。
//}

== アポロニウスのギャスケットの計算
ここからは実際のプログラムをみながら、アポロニウスのギャスケットの計算方法を順に説明していきます。
サンプルプログラムがGihubに上がっていますので、必要であればそちらからDLしてご活用ください。

URL：@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming2} 

=== 事前準備
アポロニウスのギャスケットをプログラムするにあたり、今回は円を表現するクラスと複素数を扱うための構造体を自前で用意しています。

　
//emlist[Circle.cs][cs]{
using UnityEngine;

public class Circle
{
    public float Curvature
    {
        get { return 1f / this.radius; }
    }
    public Complex Complex
    {
        get; private set;
    }
    public float Radius
    {
        get { return Mathf.Abs(this.radius); }
    }
    public Vector2 Position
    {
        get { return this.Complex.Vec2; }
    }

    private float radius = 0f;
    
    
    public Circle(Complex complex, float radius)
    {
        this.radius = radius;
        this.Complex = complex;
    }

    /// ...
    /// 以下、円同士の関係性を調べる関数が実装されています
    /// 接しているか、交わっているか、内包しているか、など
    /// ...
}
//}
円を表現するクラスの実装の一部です。

基本的なプログラミングの知識があれば難しいことは何も無いと思います。
なお、@<tt>{Complex}とは今回自前で用意した複素数構造体のことで、@<tt>{Curvature}は曲率と呼ばれるもので、どちらもアポロニウスのギャスケットを計算するのに必要な値です。

　
//emlist[Complex.cs][cs]{
using UnityEngine;
using System;
using System.Globalization;

public struct Complex
{
    public static readonly Complex Zero = new Complex(0f, 0f);
    public static readonly Complex One = new Complex(1f, 0f);
    public static readonly Complex ImaginaryOne = new Complex(0f, 1f);

    public float Real
    {
        get { return this.real; }
    }
    public float Imaginary
    {
        get { return this.imaginary; }
    }
    public float Magnitude
    {
        get { return Abs(this); }
    }
    public float SqrMagnitude
    {
        get { return SqrAbs(this); }
    }
    public float Phase
    {
        get { return Mathf.Atan2(this.imaginary, this.real); }
    }
    public Vector2 Vec2
    {
        get { return new Vector2(this.real, this.imaginary); }
    }

    [SerializeField]
    private float real;
    [SerializeField]
    private float imaginary;


    public Complex(Vector2 vec2) : this(vec2.x, vec2.y) { }

    public Complex(Complex other) : this(other.real, other.imaginary) { }

    public Complex(float real, float imaginary)
    {
        this.real = real;
        this.imaginary = imaginary;
    }

    /// ...
    /// 以下、複素数計算をする関数が実装されています
    /// 四則演算や絶対値計算など
    /// ...
}
//}
複素数を扱うための構造体です。

C#は@<tt>{Complex}構造体を有していますが.Net4.0から搭載されています。
本章を執筆している時点ではUnityの.Net4.6サポートはExperimental段階だったので、自前で用意することにしました。

=== 最初の3つの円の計算
アポロニウスのギャスケットを計算するための前提条件として、互いに接する3つの円が存在している必要があります。
そこで今回のプログラムでは、ランダムに半径を決めた3つの円を生成し、それらが互いに接する様に座標を計算して配置しています。

//emlist[ApollonianGaskets.cs][cs]{
private void CreateFirstCircles(
    out Circle c1, out Circle c2, out Circle c3)
{
    var r1 = Random.Range(
        this.firstRadiusMin, this.firstRadiusMax
    );
    var r2 = Random.Range(
        this.firstRadiusMin, this.firstRadiusMax
    );
    var r3 = Random.Range(
        this.firstRadiusMin, this.firstRadiusMax
    );

    // ランダムな座標を取得
    var p1 = this.GetRandPosInCircle(
        this.fieldRadiusMin,
        this.fieldRadiusMax
    );
    c1 = new Circle(new Complex(p1), r1);

    // p1を元に接する円の中心座標を計算
    var p2 = -p1.normalized * ((r1 - p1.magnitude) + r2);
    c2 = new Circle(new Complex(p2), r2);

    // 2つの円に接する円の中心座標を計算
    var p3 = this.GetThirdVertex(p1, p2, r1 + r2, r2 + r3, r1 + r3);
    c3 = new Circle(new Complex(p3), r3);
}

private Vector2 GetRandPosInCircle(float fieldMin, float fieldMax)
{
    // 適当な角度を取得
    var theta = Random.Range(0f, Mathf.PI * 2f);

    // 適当な距離を計算
    var radius = Mathf.Sqrt(
        2f * Random.Range(
            0.5f * fieldMin * fieldMin,
            0.5f * fieldMax * fieldMax
        )
    );

    // 極座標系からユークリッド平面に変換
    return new Vector2(
        radius * Mathf.Cos(theta),
        radius * Mathf.Sin(theta)
    );
}

private Vector2 GetThirdVertex(
    Vector2 p1, Vector2 p2, float rab, float rbc, float rca)
{
    var p21 = p2 - p1;

    // 余弦定理によって角度を計算
    var theta = Mathf.Acos(
        (rab * rab + rca * rca - rbc * rbc) / (2f * rca * rab)
    );

    // 起点となる点の角度を計算して加算
    // thetaはあくまで三角形内における角度でしかないので、平面における角度ではない
    theta += Mathf.Atan2(p21.y, p21.x);

    // 極座標系からユークリッド平面に変換した座標を起点となる座標に加算
    return p1 + new Vector2(
        rca * Mathf.Cos(theta),
        rca * Mathf.Sin(theta)
    );
}
//}

@<tt>{CreateFirstCircles}関数を呼び出すことで、初期条件の3円が生成されます。

まずランダムに3つの半径@<tt>{r1,r2,r3}を決め、次に@<tt>{GetRandPosInCircle}関数によって@<tt>{r1}の半径をもつ円（以下C1）の中心座標を決定します。
この関数は、原点中心の半径@<tt>{fieldMin}以上@<tt>{fieldMax}以下の円の内部のランダム座標を返します。

//image[aoyama/circle2][ランダムな座標が生成される領域][scale=0.8]

次に@<tt>{r2}の半径をもつ円（以下C2）の中心座標を計算します。
まず@<m>{（r1 - p1.magnitude） + r2}によって原点からC2の中心までの距離を計算します。
これを符号反転したC1の正規化座標に乗算することで、C1と隣接する@<tt>{r2}の半径をもつ円の中心座標を求めることができます。

//image[aoyama/circle3][p2ベクトルの表す位置][scale=0.8]

最後に@<tt>{r3}の半径をもつ円（以下C3）の中心座標を@<tt>{GetThirdVertex}関数にて計算しますが、この計算では@<b>{余弦定理}を用います。
読者のほとんどの方が高校で習っているかと思いますが、余弦定理というのは三角形の辺の長さと内角の余弦（cosのことです）の間に成り立つ定理のことです。
△ABCにおいて、@<m>{a = BC, b = CA, c = AB, α = ∠CAB}とすると
//texequation{
a^2 = c^2 + b^2 - 2cbcosα
//}
が成り立つというのが余弦定理です。

//image[aoyama/triangle][三角形ABC][scale=0.3]

なぜ円の中心を考えるのに三角形が必要になるのかと思われた方もいらっしゃるかと思いますが、実は3つの円の間に成り立つ関係性によって、とても使い勝手の良い三角形を考えることができます。
C1,C2,C3の各中心を頂点とする三角形を考えると、この3つの円はそれぞれが接しているので、円の半径から三角形の各辺の長さを知ることができます。

//image[aoyama/circle4][三角形ABCと円C1,C2,C3][scale=0.8]

余弦定理を変形すると
//texequation{
cosα = \frac{c^2 + b^2 - a^2}{2cb}
//}
となるため、3つの辺の長さから余弦について解くことができます。
二辺のなす角と距離が求まれば、そこからC1の中心座標を元にしてC3の中心座標を求められるようになります。。

これで、初期条件として必要な互いに接する3つの円を生成することができました。

=== C1,C2,C3に接する円の計算
前項で生成した3つの円C1,C2,C3を元にして、これらに接する円を計算します。
新しく円を作るためには半径と中心座標の二つのパラメーターが必要となるので、それぞれを計算にて求めます。

==== 半径
まず半径から計算を行いますが、これは@<b>{デカルトの円定理}によって求めることができます。
デカルトの円定理とは、互いに接する4つの円C1,C2,C3,C4について、曲率@<fn>{curvature}をそれぞれk1,k2,k3,k4とすると
//texequation{
(k_1 + k_2 + k_3 + k_4)^2 = 2({k_1}^2 + {k_2}^2 + {k_3}^2 + {k_4}^2)
//}
が成り立つというものです。
これは4つの円の半径に関する2次方程式ですが、この式を整理すると
//texequation{
k_4 = k_1 + k_2 + k_3 \pm 2\sqrt{k_1k_2 + k_2k_3 + k_3k_1}
//}

//footnote[curvature][半径の逆数のことで@<m>{k = \pm\frac{1\}{r\}}で定義されます]

と変形できるため、3つの円C1,C2,C3が分かっていれば4つ目の円C4の曲率を求められます。
曲率とは半径の逆数であるので、曲率の逆数を取ることにより円の半径を知ることができます。

ここで、C4の曲率は複合により2つ求められますが、一方の解は常に正となり、もう一方は正負のどちらかとなります。
C4の曲率が正の時はC1,C2,C3と外接し、負の時はC1,C2,C3と内接（3円を内包）しています。
つまり、4つ目の円C4は2パターン考えることができ、その両方について取り得る可能性があります。

//image[aoyama/circle5][曲率の正負][scale=0.8]

これをプログラミングしているのが以下の部分となります。
//emlist[SoddyCircles.cs][cs]{
// 曲率の計算
var k1 = this.Circle1.Curvature;
var k2 = this.Circle2.Curvature;
var k3 = this.Circle3.Curvature;

var plusK = k1 + k2 + k3 + 2f * Mathf.Sqrt(k1 * k2 + k2 * k3 + k3 * k1);
var minusK = k1 + k2 + k3 - 2f * Mathf.Sqrt(k1 * k2 + k2 * k3 + k3 * k1);
//}

なお、このデカルトの円定理は後にソディという化学者により再発見されており、C1,C2,C3,C4の円はソディの円と呼ばれています。
//note[ソディの円とアポロニウスの円]{
    前項にてアポロニウスの円という話をしましたが、これはソディの円と何が違うのかと思った方もいらっしゃるかと思います。

    アポロニウスの円というのは、アポロニウスの問題を解決する円の総称のことです。
    ソディの円というのは、デカルトの円定理を満たす4つの円を指している言葉です。

    つまり、ソディの円がアポロニウスの問題に対する解の一つであるので、アポロニウスの円でもあるということです。
//}

==== 中心座標
次に中心座標の計算ですが、デカルトの円定理と似た形の@<b>{デカルトの複素数定理}によって求められます。
デカルトの複素数定理とは、複素数平面上の互いに接する円C1,C2,C3,C4の中心座標をz1,z2,z3,z4として、曲率をk1,k2,k3,k4とすると
//texequation{
(k_1z_1 + k_2z_2 + k_3z_3 + k_4z_4)^2 = 2({k_1}^2{z_1}^2 + {k_2}^2{z_2}^2 + {k_3}^2{z_3}^2 + {k_4}^2{z_4}^2)
//}
が成り立つというものです。
この式をz4について整理すると
//texequation{
z4 = \frac{z_1k_1 + z_2k_2 + z_3k_3 \pm 2\sqrt{k_1k_2z_1z_2 + k_2k_3z_2z_3 + k_3k_1z_3z_1}}{k4}
//}
と変形できるので、これで円C4の中心座標を求めることができます。

ここで、半径の計算の時に曲率が2つ求められましたが、デカルトの複素数定理についても複合により2つ求めることができます。
ただし曲率の計算とは違い、2つのうちどちらか一方が正しいソディの円であるため、どちらが正しいかを判定する必要があります。

これをプログラミングしているのが以下の部分となります。
//emlist[SoddyCircles.cs][cs]{
/// 中心座標の計算
var ck1 = Complex.Multiply(this.Circle1.Complex, k1);
var ck2 = Complex.Multiply(this.Circle2.Complex, k2);
var ck3 = Complex.Multiply(this.Circle3.Complex, k3);

var plusZ = ck1 + ck2 + ck3
    + Complex.Multiply(Complex.Sqrt(ck1 * ck2 + ck2 * ck3 + ck3 * ck1), 2f);
var minusZ = ck1 + ck2 + ck3
    - Complex.Multiply(Complex.Sqrt(ck1 * ck2 + ck2 * ck3 + ck3 * ck1), 2f);

var recPlusK = 1f / plusK;
var recMinusK = 1f / minusK;

// ソディの円の判定
this.GetGasket(
    new Circle(Complex.Divide(plusZ, plusK), recPlusK),
    new Circle(Complex.Divide(minusZ, plusK), recPlusK),
    out c4
);

this.GetGasket(
    new Circle(Complex.Divide(plusZ, minusK), recMinusK),
    new Circle(Complex.Divide(minusZ, minusK), recMinusK),
    out c5
);
//}

//emlist[SoddyCircles.cs][cs]{
/// ソディの円の判定
(c1.IsCircumscribed(c4, CalculationAccuracy)
    || c1.IsInscribed(c4, CalculationAccuracy)) &&
(c2.IsCircumscribed(c4, CalculationAccuracy)
    || c2.IsInscribed(c4, CalculationAccuracy)) &&
(c3.IsCircumscribed(c4, CalculationAccuracy)
    || c3.IsInscribed(c4, CalculationAccuracy))
//}

//emlist[Circle.cs][cs]{
public bool IsCircumscribed(Circle c, float accuracy)
{
    var d = (this.Position - c.Position).sqrMagnitude;
    var abs = Mathf.Abs(d - Mathf.Pow(this.Radius + c.Radius, 2));

    return abs <= accuracy * accuracy;
}

public bool IsInscribed(Circle c, float accuracy)
{
    var d = (this.Position - c.Position).sqrMagnitude;
    var abs = Mathf.Abs(d - Mathf.Pow(this.Radius - c.Radius, 2));

    return abs <= accuracy * accuracy;
}
//}

　

これで、初期条件のC1,C2,C3を元にして、これらに接する円を2つ（以下C4,C5）得ることができました。

=== アポロニウスのギャスケットの計算
ここまで来たら、あとは簡単にアポロニウスのギャスケットを計算することができます。
@<hd>{C1,C2,C3に接する円の計算}で行った計算を繰り返すだけです。

前項では@<hd>{最初の3つの円の計算}で求めたC1,C2,C3について接する円C4,C5を求めました。
次は（C1,C2,C4）（C1,C2,C5）（C2,C3,C4）（C2,C3,C5）（C3,C1,C4）（C3,C1,C5）について接する円をそれぞれ求めていきます。

ここで、組み合わせの上では接している円であっても、実際には他の円に重なってしまっている可能性があります。
なので正しいソディの円であるかの判定を行ったあとに、今まで求めてきた全ての円に重なっていないことも確認する必要があります。

//image[aoyama/circle6][C1,C4,C6に接する円C7,C8のうち、C8はC2と重なっているためアポロニウスのギャスケットには含まれない][scale=0.8]

そうして、それぞれについて接する円を新たに得ることができます。
あとは同じよう、接する円を求めるために元とした円と、新しく求めた接する円との全ての組み合わせについて、改めて新しい接する円を求め続けます。

数学的にはこの手順を無限回繰り返した時に得られる円の集合がアポロニウスのギャスケットなのですが、プログラムについて無限回を扱うことはできません。
なので、今回のプログラムにおいては、新しく求めた接する円の半径が一定値以下だった場合は、その組み合わせについては処理を終了という条件を与えています。

これをプログラミングしているのが以下の部分となります。
//emlist[ApollonianGaskets.cs][cs]{
private void Awake()
{
    // 初期条件の3円を生成
    Circle c1, c2, c3;
    this.CreateFirstCircles(out c1, out c2, out c3);
    this.circles.Add(c1);
    this.circles.Add(c2);
    this.circles.Add(c3);

    this.soddys.Enqueue(new SoddyCircles(c1, c2, c3));

    while(this.soddys.Count > 0)
    {
        // ソディの円を計算
        var soddy = this.soddys.Dequeue();

        Circle c4, c5;
        soddy.GetApollonianGaskets(out c4, out c5);

        this.AddCircle(c4, soddy);
        this.AddCircle(c5, soddy);
    }
}

private void AddCircle(Circle c, SoddyCircles soddy)
{
    if(c == null || c.Radius <= MinimumRadius)
    {
        return;
    }
    // 曲率が負の場合は問答無用で追加
    // 曲率が負の円は一度しか出てこない
    else if(c.Curvature < 0f)
    {
        this.circles.Add(c);
        soddy.GetSoddyCircles(c).ForEach(s => this.soddys.Enqueue(s));

        return;
    }

    // 他の円と被ってないか確認
    for(var i = 0; i < this.circles.Count; i++)
    {
        var o = this.circles[i];
        
        if(o.Curvature < 0f)
        {
            continue;
        }
        else if(o.IsMatch(c, CalculationAccuracy) == true)
        {
            return;
        }
    }

    this.circles.Add(c);
    soddy.GetSoddyCircles(c).ForEach(s => this.soddys.Enqueue(s));
}
//}

これで無事アポロニウスのギャスケットを求めることができました。

//image[aoyama/gasket][Unity上での実行結果][scale=1.0]

== まとめ
ここまでアポロニウスのギャスケットを計算するために必要な手順を順序的に説明してきました。
冒頭でも説明したましたが、本来アポロニウスのギャスケットというのはフラクタル図形という意味合いの方が強いです。

しかし、今回の平面という制限を外し空間という世界に飛び出すと、途端に話が難しくなり、フラクタル図形から充填（パッキング）としての意味合いが強くなります。
空間を球充填するという命題は、ケプラー予想などの有名な数学的予想が存在するなど、何百年に渡って議論の的とされてきた分野です。

Space filling問題についても、実用的な面で有用な物です。
VLSIのレイアウト設計の最適化、布などの部材切り出しの最適化、UV展開の自動化と最適化など、幅広い分野で応用されています。

今回はアポロニウスのギャスケットという比較的理解しやすく見た目的にも面白い物を選びました。
もしパッキングそのものに対する興味が湧いてきましたら、冒頭で紹介したアルゴリズムなどを調べてみてください。

物体内部を物体で埋め尽くす。
というのは、意外な所で新しい表現方法として活用できると思います。

== 参考

 * https://ja.wikipedia.org/wiki/アポロニウスのギャスケット
 * https://ja.wikipedia.org/wiki/デカルトの円定理
 * http://paulbourke.net/fractals/randomtile/

