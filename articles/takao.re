= Shape Matching - 線形代数のCGへの応用

== はじめに
本章では, 線形代数学の基礎・応用から, 特異値分解の学習, 特異値分解を用いたアプリケーションの紹介を行います.
高校生で行列, 大学生で線形代数を学んだものの, 「CGの世界でどのように生かされているのかがわからない」という方も多いのではないかと思い, 今回の執筆にあたりました.
本章では, 理解しやすさを優先するために, @<b>{2次元かつ実数の範囲内}での解説を行います.
そのため, 実際の線形代数の定義とは少し異なる部分がありますが, 適宜読み替えていただけると幸いです.

== 行列とは？
ほとんどの読者の皆様は, 1度は行列という言葉を耳にしたことがあるかと思います(現在, 高校では行列を習わないのだとか...).
行列とは, 以下のように数字を縦と横に並べたものを指します.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $M$} = \left(
    \begin{array}{cc}
      1 & 2 \\
      3 & 4
    \end{array}
  \right)\
  \end{equation}
//}
横方向を@<b>{行}, 縦方向を@<b>{列}, 斜め方向を@<b>{対角}, それぞれの数字を行列の@<b>{要素}と呼びます.
//image[matrix][行列][scale=0.2]{
//}

ちなみに, 行列は, 英語でMatrix(マトリックス)と呼ばれます.

== 行列演算のおさらい
行列演算の基本について軽くおさらいしていきましょう. 既にご存じの方は, この節を読み飛ばしていただいて構いません.

=== 加減乗除
行列には, スカラーの四則演算と同様, 加減乗除算が存在します.
簡単のために, 2次正方行列@<fn>{square}@<m>{\mbox{\boldmath $A$\}}と@<m>{\mbox{\boldmath $B$\}}, 2次元ベクトル@<m>{\mbox{\boldmath $c$\}}を以下で定義しておきます.

//footnote[square][正方行列 @<m>{\cdots} 行と列の数が等しい行列のこと.]

//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $A$} = \left(
    \begin{array}{cc}
      a_{00} & a_{01} \\
      a_{10} & a_{11}
    \end{array}
  \right), 
  \mbox{\boldmath $B$} = \left(
    \begin{array}{cc}
      b_{00} & b_{01} \\
      b_{10} & b_{11}
    \end{array}
  \right), 
  \mbox{\boldmath $c$} = \left(
    \begin{array}{cc}
      c_{0} \\
      c_{1}
    \end{array}
  \right)
  \end{equation}
//}

==== 加算
行列の加算は, 式(@<raw>{\ref{plus\}})のように, 要素ごとに和を計算します.
//embed[latex]{
  \begin{equation}
  \label{plus}
  \mbox{\boldmath $A$} + \mbox{\boldmath $B$} = \left(
    \begin{array}{cc}
      a_{00} + b_{00} & a_{01} + b_{01} \\
      a_{10} + b_{10} & a_{11} + b_{11}
    \end{array}
  \right)
  \end{equation}
//}

==== 減算
行列の減算は, 式(@<raw>{\ref{minus\}})のように, 要素ごとに差を計算します.
//embed[latex]{
  \begin{equation}
  \label{minus}
  \mbox{\boldmath $A$} - \mbox{\boldmath $B$} = \left(
    \begin{array}{cc}
      a_{00} - b_{00} & a_{01} - b_{01} \\
      a_{10} - b_{10} & a_{11} - b_{11}
    \end{array}
  \right)
  \end{equation}
//}

==== 乗算
行列の乗算は少し複雑で, 式(@<raw>{\ref{times\}})のようになります.
//embed[latex]{
  \begin{equation}
  \label{times}
  \mbox{\boldmath $A$} \mbox{\boldmath $B$} = \left(
    \begin{array}{cc}
      a_{00} b_{00} + a_{01} b_{10} & a_{00} b_{01} + a_{01} b_{11} \\
      a_{10} b_{00} + a_{11} b_{10} & a_{10} b_{01} + a_{11} b_{11}
    \end{array}
  \right)
  \end{equation}
//}
掛ける順序を逆にすると, 計算結果も変化するので, 注意してください.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $B$} \mbox{\boldmath $A$} = \left(
    \begin{array}{cc}
      b_{00} a_{00} + b_{01} a_{10} & b_{00} a_{01} + b_{01} a_{11} \\
      b_{10} a_{00} + b_{11} a_{10} & b_{10} a_{01} + b_{11} a_{11}
    \end{array}
  \right)
  \end{equation}
//}

==== 除算(逆行列)
行列の除算は, スカラーにおける除算と少し異なる, @<b>{逆行列}と呼ばれる概念を使用します.
まずはじめに, スカラーでは, 自分自身の逆数を掛けると, 必ず1になる性質を持っています.
//embed[latex]{
  \begin{equation}
  4 \times \frac{1}{4} = 1
  \end{equation}
//}

つまり, 除算という行為は, 「逆数を掛ける」という演算と等しくなります.
//embed[latex]{
  \begin{equation}
  7 \times \frac{1}{4} = \frac{7}{4}
  \end{equation}
//}

これを行列に置き換えると, 行列にかけて単位行列を生み出すものが, 除算を表す行列であるということができます.
行列において, スカラーの@<m>{1}に対応するものは単位行列と呼ばれ, 以下で定義されます.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $I$} = \left(
    \begin{array}{cc}
      1 & 0 \\
      0 & 1
    \end{array}
  \right)
  \end{equation}
//}
スカラーの@<m>{1}と同様, 単位行列をどのような行列にかけても, 値が変化しません.

これらを踏まえて, 行列の除算について考えます.
逆行列を@<m>{\mbox{\boldmath $M$\}^{-1\}}とすると, 逆行列の定義は以下となります.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $M$} \mbox{\boldmath $M$}^{-1} = \mbox{\boldmath $M$}^{-1} \mbox{\boldmath $M$} = \mbox{\boldmath $I$}
  \end{equation}
//}

導出は省略しますが, 行列@<m>{\mbox{\boldmath $A$\}}の逆行列の要素は, 以下で定義されます.

//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $A$}^{-1} = \frac{1}{a_{00}a_{11} - a_{01}a_{10}} \left(
    \begin{array}{cc}
      a_{11} & -a_{01} \\
      -a_{10} & a_{00}
    \end{array}
  \right)
  \end{equation}
//}

この時の, @<m>{a_{00\}a_{11\} - a_{01\}a_{10\}}を行列式(Determinant)と呼び, @<m>{det(\mbox{\boldmath $A$\})}と表します.


#@#余談ですが, 逆行列を持つ行列のことを, 正則行列と呼びます.

=== 行列のベクトルへの作用
行列は, ベクトルと乗算することによってベクトルの指す座標を変換することができます.
ご存知の通り, CGでは, 座標変換行列(ワールド, プロジェクション, ビュー変換行列)などとして用いられることがほとんどです.
行列とベクトルの積は, 以下で定義されます.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $A$} \mbox{\boldmath $c$} = \left(
    \begin{array}{c}
      a_{00} c_{0} + a_{01} c_{1} \\
      a_{10} c_{0} + a_{11} c_{1}
    \end{array}
  \right)
  \end{equation}
//}

== 行列演算の応用
本節からは, 大学で習う範囲の行列の概念を解説していきます.
少々難しいと感じられる部分が多いかとは思いますが, Shape Matchingを理解するには, これだけの概念が必要ですので, 頑張って理解していきましょう.
と言いつつ, 本節の話は行列演算ライブラリ内部で吸収される部分でもありますので, 第@<raw>{\ref{shapematching\}}節まで読み飛ばしていただいても実装に支障はありません.

#@#=== 行列式
#@#逆行列を算出する際に一度出現していますが, すべての正方行列には行列式というものがあり, 以下で定義されます.


=== 転置行列
転置行列は, 要素の行と列を入れ替えたもので, 以下で定義されます.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $A$}^{T} = \left(
    \begin{array}{cc}
      a_{00} & a_{10}\\
      a_{01} & a_{11}
    \end{array}
  \right)
  \end{equation}
//}

=== 対称行列
@<m>{\mbox{\boldmath $A$\}^{T\} = \mbox{\boldmath $A$\}}を満たす行列を, 対称行列と呼びます.

=== 固有値と固有ベクトル
正方行列@<m>{\mbox{\boldmath $A$\}}が与えられたとき,
//embed[latex]{
  \begin{equation}
  \label{eigen}
  \mbox{\boldmath $A$} \mbox{\boldmath $v$} = \lambda \mbox{\boldmath $v$}, (\mbox{\boldmath $v$} \neq 0)
  \end{equation}
//}
を満たす@<m>{\lambda}を, @<m>{\mbox{\boldmath $A$\}}の@<b>{固有値}, @<m>{\mbox{\boldmath $v$\}}を@<b>{固有ベクトル}と呼びます.

固有値と固有ベクトルの算出方法を以下に示します.
まず, 式(@<raw>{\ref{eigen\}})を変形して,
//embed[latex]{
  \begin{equation}
  \label{eigen2}
  (\mbox{\boldmath $A$} - \lambda \mbox{\boldmath $I$})\mbox{\boldmath $v$} = 0
  \end{equation}
//}
とします.
ここで, @<m>{\mbox{\boldmath $v$\} \neq 0}という条件を用いると, 式(@<raw>{\ref{eigen2\}})は,
//embed[latex]{
  \begin{equation}
  det(\mbox{\boldmath $A$} - \lambda \mbox{\boldmath $I$}) = 0
  \end{equation}
//}
となります.
この式を展開すると, @<m>{\lambda}についての2次方程式になるため, これを解くことで@<m>{\lambda}を算出することができます.
また, 算出されたそれぞれの@<m>{\lambda}を式(@<raw>{\ref{eigen2\}})に代入することで, 固有ベクトル@<m>{\mbox{\boldmath $v$\}}を算出することができます.

数式だけでは, 固有値・固有ベクトルの概念はわかりにくいので, @kenmatsu4氏による固有値の可視化が行われたQiita記事(章末に記載)を併せてご覧いただけるとよいかと思います.

=== 固有値分解
正方行列@<m>{\mbox{\boldmath $A$\}}における固有値と固有ベクトルを用いて, 行列@<m>{\mbox{\boldmath $A$\}}を違う形で表すことができます.
まず, 固有値@<m>{\lambda}を大きさ順に並べ替え, それを対角要素に持つ行列@<m>{\mbox{\boldmath $\Lambda$\}}を作成します.
次に, それぞれの固有値に対応する固有ベクトルを左から順に並べた行列@<m>{\mbox{\boldmath $V$\}}を作成します.
すると, 式(@<m>{\ref{eigen\}})を, これらの行列を用いて以下のように書き換えることができます.

//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $A$} \mbox{\boldmath $V$} = \mbox{\boldmath $V$} \mbox{\boldmath $\Lambda$}
  \end{equation}
//}

さらに, これを左辺に行列@<m>{\mbox{\boldmath $A$\}}が残るよう, 両辺右から@<m>{\mbox{\boldmath $V$\}^{-1\}}を掛けると,
//embed[latex]{
  \begin{equation}
  \label{eigendecomp}
  \mbox{\boldmath $A$} = \mbox{\boldmath $V$} \mbox{\boldmath $\Lambda$} \mbox{\boldmath $V$}^{-1}
  \end{equation}
//}
となります.

このように, 行列を式(@<raw>{\ref{eigendecomp\}})のような形式に分解することを, 行列の固有値分解と呼びます.


=== 正規直交基底
互いに垂直であり, かつそれぞれが単位ベクトルであるベクトルの組を, 正規直交基底と呼びます.
任意のベクトルは, 正規直交基底の組を用いて表すことができます@<fn>{ichi}.
2次元の場合, 正規直交基底となりうるベクトルは2つとなります.
例えば, よく用いられるx軸とy軸は, @<m>{\mbox{\boldmath $x$\} = (1, 0), \mbox{\boldmath $y$\} = (0, 1)}の組で正規直交基底を成しているので,
任意のベクトルをこの@<m>{\mbox{\boldmath $x$\}, \mbox{\boldmath $y$\}}で表すことができます.
@<m>{\mbox{\boldmath $v$\} = (4, 13)}を正規直交基底@<m>{\mbox{\boldmath $x$\}, \mbox{\boldmath $y$\}}を用いて表すと,
@<m>{\mbox{\boldmath $v$\} = 4\mbox{\boldmath $x$\} + 13\mbox{\boldmath $y$\}}となります.

//footnote[ichi][正式にはベクトルの一次結合と呼びます.]

=== エルミート行列
エルミート行列が定義されるのは, 本来複素数の範囲であり本章の領域を超えますので, 実数の範囲で簡単に説明します.
実数の範囲では, 行列@<m>{\mbox{\boldmath $A$\}}のエルミート行列@<m>{\mbox{\boldmath $A$\}^{*\}}は, 単に対称行列であることを意味しており,
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $A$}^{*} = (\overline{\mbox{\boldmath $A$}})^{T} = \mbox{\boldmath $A$}^{T}
  \end{equation}
//}
となります.

=== 直交行列
正方行列@<m>{\mbox{\boldmath $Q$\}}を列ベクトル @<m>{Q = (\mbox{\boldmath $q$\}_1, \mbox{\boldmath $q$\}_2, \cdots, \mbox{\boldmath $q$\}_n)}に分解したとき, これらのベクトルの組が正規直交系をなしている, つまり,
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $Q$}^{T} \mbox{\boldmath $Q$} = \mbox{\boldmath $I$}, \mbox{\boldmath $Q$}^{-1} = \mbox{\boldmath $Q$}^{T}
  \end{equation}
//}
が成り立つとき, @<m>{\mbox{\boldmath $Q$\}}は直交行列であるといいます.
また, 直交行列を行ベクトルに分解したとしても, 正規直交系をなしている特徴があります.

=== ユニタリ行列
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $U$}^{*} \mbox{\boldmath $U$} = \mbox{\boldmath $U$} \mbox{\boldmath $U$}^{*} = \mbox{\boldmath $I$}
  \end{equation}
//}
を満たす行列を, ユニタリ行列と呼びます.
ユニタリ行列@<m>{\mbox{\boldmath $U$\}}の要素がすべて実数(実行列)の場合, @<m>{\mbox{\boldmath $U$\}^{*\} = \mbox{\boldmath $U$\}^{T\}}となるため, 実ユニタリ行列@<m>{\mbox{\boldmath $U$\}}は直交行列であることがわかります.

== 特異値分解
任意の@<m>{m \times n}行列@<m>{\mbox{\boldmath $A$\}}を, 以下の形に分解することを, 行列の特異値分解と呼びます.
//embed[latex]{
  \begin{equation}
  \label{svd}
  \mbox{\boldmath $A$} = \mbox{\boldmath $U$} \mbox{\boldmath $\Sigma$} \mbox{\boldmath $V$}^{T}
  \end{equation}
//}
尚, @<m>{\mbox{\boldmath $U$\}}と@<m>{\mbox{\boldmath $V$\}^{T\}}は, @<m>{m \times m}の直交行列,
@<m>{\mbox{\boldmath $\Sigma$\}}は, @<m>{m \times n}の対角行列(対角成分は非負で大きさの順に並んだ行列)となっています.

「任意の」という言葉が肝で, 行列の固有値分解は正方行列に対してのみ定義されますが, 特異値分解は長方行列でも行うことができます.
CGの世界では, 扱う行列が正方行列であることがほとんどですので, 固有値分解とさほど計算方法は変わりません.
また, @<m>{\mbox{\boldmath $A$\}}が対称行列のとき, @<m>{\mbox{\boldmath $A$\}}の固有値と特異値は一致します.
さらに, @<m>{\mbox{\boldmath $A$\}^{T\} \mbox{\boldmath $A$\}}の@<m>{0}でない固有値の正の平方根は@<m>{\mbox{\boldmath $A$\}}の特異値です.

=== 特異値分解のアルゴリズム
固有値分解をプログラムに落とし込むには, 式(@<raw>{\ref{svd\}})を式変形するとわかりやすいです.
行列@<m>{\mbox{\boldmath $A$\}}の左からその行列の転置@<m>{\mbox{\boldmath $A$\}^{T\}}を掛けると, 以下のようになります.

//embed[latex]{
  \begin{eqnarray}
  \mbox{\boldmath $A$}^{T} \mbox{\boldmath $A$}
  &=& (\mbox{\boldmath $U$} \mbox{\boldmath $\Sigma$} \mbox{\boldmath $V$}^{T})^{T} \mbox{\boldmath $U$} \mbox{\boldmath $\Sigma$} \mbox{\boldmath $V$}^{T} \\
  &=& (\mbox{\boldmath $V$} \mbox{\boldmath $\Sigma$}^{T} \mbox{\boldmath $U$}^{T}) \mbox{\boldmath $U$} \mbox{\boldmath $\Sigma$} \mbox{\boldmath $V$}^{T} \\
  &=& \mbox{\boldmath $V$} \mbox{\boldmath $\Sigma$}^{T} \mbox{\boldmath $\Sigma$} \mbox{\boldmath $V$}^{T} \\
  &=& \mbox{\boldmath $V$} \mbox{\boldmath $\Sigma$}^{2} \mbox{\boldmath $V$}^{T}
  \end{eqnarray}
//}

形が固有値分解と同じになっているのに気づくかと思います.
実際, 特異値行列の自乗は固有値行列になることが知られています.
よって, 特異値の算出は, 行列を固有値分解し, 固有値の2乗根を取ることで行うことが可能です.
このことから, 固有値分解をアルゴリズム内に組み込むことになりますが, 固有値を求めるために2次方程式を解く必要が出てきます.
幸い, 2次方程式は解の公式が単純ですので, プログラムに落とすのも簡単です@<fn>{kai}.

//footnote[kai][3次方程式や4次方程式にも解の公式は存在しますが, 一般的にニュートン法等を用いて計算を行います. ]

@<m>{\mbox{\boldmath $A$\}^{T\}\mbox{\boldmath $A$\}}固有値分解することで, @<m>{\mbox{\boldmath $\Sigma$\}}と@<m>{\mbox{\boldmath $V$\}^{T\}}が算出できましたので, 残りの@<m>{\mbox{\boldmath $U$\}}は式(@<raw>{\ref{svd\}})を変形して以下で算出できます.
//embed[latex]{
  \begin{eqnarray}
  \mbox{\boldmath $U$}
  &=& \mbox{\boldmath $A$} \mbox{\boldmath $\Sigma$}^{-1} (\mbox{\boldmath $V$}^{T})^{-1} \\
  &=& \mbox{\boldmath $A$} \mbox{\boldmath $\Sigma$}^{-1} \mbox{\boldmath $V$} \\
  \end{eqnarray}
//}
尚, Vは直交行列なので, 転置と逆行列が一致します.

これをプログラムで表すと, 以下のようになります.



//listnum[render][特異値分解のアルゴリズム(Matrix2x2.cs)][csharp]{
/// <summary>
/// 特異値分解
/// </summary>
/// <param name="u">Returns rotation matrix u</param>
/// <param name="s">Returns sigma matrix</param>
/// <param name="v">Returns rotation matrix v(not transposed)</param>
public void SVD(ref Matrix2x2 u, ref Matrix2x2 s, ref Matrix2x2 v)
{
    // 対角行列であった場合、特異値分解は単純に以下で与えられる。
    if (Mathf.Abs(this[1, 0] - this[0, 1]) < MATRIX_EPSILON 
        && Mathf.Abs(this[1, 0]) < MATRIX_EPSILON)
    {
        u.SetValue(this[0, 0] < 0 ? -1 : 1, 0, 
                    0, this[1, 1] < 0 ? -1 : 1);
        s.SetValue(Mathf.Abs(this[0, 0]), Mathf.Abs(this[1, 1]));
        v.LoadIdentity();
    }

    // 対角行列でない場合、A^T*Aを計算する。
    else
    {
        // 0列ベクトルの長さ(非ルート)
        float i   = this[0, 0] * this[0, 0] + this[1, 0] * this[1, 0];
        // 1列ベクトルの長さ(非ルート)
        float j   = this[0, 1] * this[0, 1] + this[1, 1] * this[1, 1];
        // 列ベクトルの内積
        float i_dot_j = this[0, 0] * this[0, 1] 
                          + this[1, 0] * this[1, 1];

        // A^T*Aが直交行列であった場合
        if (Mathf.Abs(i_dot_j) < MATRIX_EPSILON)
        {
            // 特異値行列の対角要素の計算
            float s1 = Mathf.Sqrt(i);
            float s2 = Mathf.Abs(i - j) < 
                        MATRIX_EPSILON ? s1 : Mathf.Sqrt(j);

            u.SetValue(this[0, 0] / s1, this[0, 1] / s2, 
                        this[1, 0] / s1, this[1, 1] / s2);
            s.SetValue(s1, s2);
            v.LoadIdentity();
        }
        // A^T*Aが直交行列でない場合、固有値を求めるために二次方程式を解く。
        else
        {
            // 固有値/固有ベクトルの算出
            float i_minus_j = i - j;    // 列ベクトルの長さの差
            float i_plus_j = i + j;     // 列ベクトルの長さの和

            // 2次方程式の解の公式
            float root = Mathf.Sqrt(i_minus_j * i_minus_j 
                                      + 4 * i_dot_j * i_dot_j);
            float eig = (i_plus_j + root) * 0.5f;
            float s1 = Mathf.Sqrt(eig);
            float s2 = Mathf.Abs(root) < 
                        MATRIX_EPSILON ? s1 : 
                          Mathf.Sqrt((i_plus_j - root) / 2);

            s.SetValue(s1, s2);

            // A^T*Aの固有ベクトルをVとして用いる。
            float v_s = eig - i;
            float len = Mathf.Sqrt(v_s * v_s + i_dot_j * i_dot_j);
            i_dot_j /= len;
            v_s /= len;
            v.SetValue(i_dot_j, -v_s, v_s, i_dot_j);

            // vとsが算出済みなので、回転行列uをAv/sで算出
            u.SetValue(
                (this[0, 0] * i_dot_j + this[0, 1] * v_s) / s1,
                (this[0, 1] * i_dot_j - this[0, 0] * v_s) / s2,
                (this[1, 0] * i_dot_j + this[1, 1] * v_s) / s1,
                (this[1, 1] * i_dot_j - this[1, 0] * v_s) / s2
            );
        }
    }
}
//}

== 特異値分解を用いるアルゴリズム
特異値分解は, 多種多様な分野で活躍しており, 主に統計学における主成分分析(PCA)で用いられることが多いようです.
CGで利用される例も少なくはなく,

 * Shape Matching@<fn>{sm}
 * Anisotropic Kernel@<fn>{ak}
 * Material Point Method@<fn>{mpm}
などが挙げられます.

今回は, Shape Matchingにフォーカスを当て, 基礎的な考え方について解説していきます.

//footnote[sm][Meshless deformations based on shape matching, Matthias Muller et al., SIGGRAPH 2005 ]
//footnote[ak][Reconstructing surfaces of particle-based fluids using anisotropic kernels, Jihun Yu et al., ACM Transaction on Graphics 2013]
//footnote[mpm][A material point method for snow simulation, Alexey Stomakhin et al., SIGGRAPH 2013]

== Shape Matching
@<raw>{\label{shapematching\}}

=== 概要
Shape Matchingとは, 異なる2つの形状を, 極力誤差のない範囲で整列させる技術を指します.
現在では, Shape Matchingを用いて, 弾性体を疑似的にシミュレートするような手法も開発されています.

本節では, @<img>{aim}, @<img>{result}のように, 
ユニコーンのオブジェクトの配置を, ライオンのオブジェクトの配置へ整列するアルゴリズムを解説します.
//image[aim][2つのオブジェクト]{
//}
//image[result][整列した結果][scale=0.55]{
//}

=== アルゴリズム
初めに, それぞれの形状の上に同数の点の集合を定義します. (ライオンの点集合をP, ユニコーンの点集合をQとします.)
//embed[latex]{
  \begin{equation}
  P = \left\{ \mbox{\boldmath $p$}_1, \mbox{\boldmath $p$}_2, \cdots, \mbox{\boldmath $p$}_n \right\} , Q = \left\{ \mbox{\boldmath $q$}_1, \mbox{\boldmath $q$}_2, \cdots, \mbox{\boldmath $q$}_n \right\}
  \end{equation}
//}
このとき, 下添え字が同一のものは, @<img>{point}のように, 幾何学的に対応した位置にあることに注意してください.
//image[point][点集合の対応]{
//}

次に, それぞれの点集合の重心を計算します.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $p$} = \frac{1}{n} \sum_{i = 1}^{n} \mbox{\boldmath $p$}_i
  \end{equation}
//}
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $q$} = \frac{1}{n} \sum_{i = 1}^{n} \mbox{\boldmath $q$}_i
  \end{equation}
//}

ユニコーンの点集合の重心が, ライオンの点集合の重心と同じ位置に来るとすると,
ユニコーンの点集合に回転行列@<m>{\mbox{\boldmath $R$\}}を作用させ, ベクトル@<m>{\mbox{\boldmath $t$\}}平行移動させた結果が,
ライオンの重心と等しくなることから, 以下の式が導出できます.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $p$} = \frac{1}{n} \sum_{i = 1}^{n} \mbox{\boldmath $R$} \mbox{\boldmath $q$}_i + \mbox{\boldmath $t$}
  \end{equation}
//}

これを変形すると,
//embed[latex]{
  \begin{eqnarray}
  \mbox{\boldmath $p$} &=& \mbox{\boldmath $R$}(\frac{1}{n} \sum_{i = 1}^{n} \mbox{\boldmath $q$}_i) + \mbox{\boldmath $t$} \\
  &=& \mbox{\boldmath $R$} \mbox{\boldmath $q$} + \mbox{\boldmath $t$}
  \end{eqnarray}
//}

となり, さらに変形して,

//embed[latex]{
  \begin{equation}
  \label{trans}
  \mbox{\boldmath $t$} = \mbox{\boldmath $p$} - \mbox{\boldmath $R$} \mbox{\boldmath $q$}
  \end{equation}
//}
となります.

よって, この式から, 回転行列@<m>{\mbox{\boldmath $R$\}}が求まれば, 自動的に平行移動ベクトル@<m>{\\mbox{\boldmath $t$\}}が求まることがわかります.
ここで, もともとの点の位置から, それぞれの重心を引いた点集合を定義します.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $p$}_{i}^{\prime} = \mbox{\boldmath $p$}_{i} - \mbox{\boldmath $p$}, \mbox{\boldmath $q$}_{i}^{\prime} = \mbox{\boldmath $q$}_{i} - \mbox{\boldmath $q$}
  \end{equation}
//}
これにより, それぞれの点集合の重心を原点としたローカル座標で計算を行うことができるようになります.

次に, @<m>{\mbox{\boldmath $p$\}_{i\}^{\prime\}, \mbox{\boldmath $q$\}_{i\}^{\prime\}}の分散共分散行列@<m>{\mbox{\boldmath $H$\}}を計算します.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $H$} = \sum_{i = 1}^{n} \mbox{\boldmath $q$}_{i}^{\prime} {\mbox{\boldmath $p$}_{i}^{\prime}}^{T}
  \end{equation}
//}
この分散共分散行列@<m>{\mbox{\boldmath $H$\}}には, 2つの点集合のばらつき具合などの情報が格納されます.
ここでのベクトルの積@<m>{\mbox{\boldmath $q$\}_{i\}^{\prime\} {\mbox{\boldmath $p$\}_{i\}^{\prime\}\}^{T\}}は, 通常のベクトルの内積演算とは異なり, 直積(outer product)と呼ばれる演算となります.
ベクトル同士の直積をとると, 行列が生成されます.
2次元ベクトルでの直積は, 以下で定義されています.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $a$} \mbox{\boldmath $b$}^{T}
  = \left(
    \begin{array}{cc}
      a_{0}b_{0} & a_{0} b_{1} \\
      a_{1}b_{0} & a_{1} b_{1}
    \end{array}
  \right)
  \end{equation}
//}

さらに, 分散共分散行列@<m>{\mbox{\boldmath $H$\}}を特異値分解します.
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $H$} = \mbox{\boldmath $U$} \mbox{\boldmath $\Sigma$} \mbox{\boldmath $V$}^{T}
  \end{equation}
//}

特異値分解をした結果の中で, @<m>{\mbox{\boldmath $\sum$\}}は伸縮を表す行列であるため,
求める回転行列@<m>{\mbox{\boldmath $R$\}}は,
//embed[latex]{
  \begin{equation}
  \mbox{\boldmath $R$} = \mbox{\boldmath $V$} \mbox{\boldmath $U$}^{T}
  \end{equation}
//}
となります. (詳細な導出方法はやや高度となりますので, ここでは省略させていただきます.)

最後に, 求めた回転行列と式(@<raw>{\ref{trans\}})から, 平行移動ベクトル@<m>{\mbox{\boldmath $t$\}}を算出することができます.

=== 実装
今回の実装は, 前節でのアルゴリズムをほぼそのままコードに落とすだけなので, 詳しい説明は省略させていただきます.
尚, ShapeMaching.cs内のStart関数内ですべての処理を完結させています.

//listnum[shapematching_impl][ShapeMatching(ShapeMaching.cs)][csharp]{
// Set p, q
p = new Vector2[n];
q = new Vector2[n];
centerP = Vector2.zero;
centerQ = Vector2.zero;

for(int i = 0; i < n; i++)
{
    Vector2 pos = _destination.transform.GetChild(i).position;
    p[i] = pos;
    centerP += pos;

    pos = _target.transform.GetChild(i).position;
    q[i] = pos;
    centerQ += pos;
}
centerP /= n;
centerQ /= n;

// Calc p', q'
Matrix2x2 H = new Matrix2x2(0, 0, 0, 0);
for (int i = 0; i < n; i++)
{
    p[i] = p[i] - centerP;
    q[i] = q[i] - centerQ;

    H += Matrix2x2.OuterProduct(q[i], p[i]);
}

Matrix2x2 u = new Matrix2x2();
Matrix2x2 s = new Matrix2x2();
Matrix2x2 v = new Matrix2x2();
H.SVD(ref u, ref s, ref v);

R = v * u.Transpose();
Debug.Log(Mathf.Rad2Deg * Mathf.Acos(R.m00));
t = centerP - R * centerQ;
//}


== 結果
無事, ユニコーンの形状をライオンの形状に整列することができました.
//image[before][実行前][scale=0.8]{
//}
//image[res][実行後][scale=0.8]{
//}


== まとめ
本節では, 特異値分解を使用したShape Matching法の実装について解説しました.
今回は2次元での実装でしたが, 3次元での実装も同じアルゴリズムで行うことができます. 
説明が至らない部分も多々あったかと思いますが, これを機に, 行列演算のCG分野での応用方法に興味を持っていただき, 学習を深めていただければ幸いです.

== References

 * 3D Geometry for Computer Graphics (@<href>{https://igl.ethz.ch/teaching/tau/cg/cg2005/svd.ppt})
 * 理工系の数理 線形代数 (永井敏隆, 永井敦 著) 裳華房
 * Singular Value Decomposition (the SVD) : MIT OpenCourseWare (@<href>{https://www.youtube.com/watch?v=mBcLRGuAFUk})
 * Lecture: The Singular Value Decomposition (SVD) : AMATH 301 (@<href>{https://www.youtube.com/watch?v=EokL7E6o1AE})
 * 固有値・固有ベクトルとは何かを可視化してみる @kenmatsu4 (@<href>{https://qiita.com/kenmatsu4/items/2a8573e3c878fc2da306})