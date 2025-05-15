### 这是基于[tinyrenderer](https://github.com/ssloy/tinyrenderer "tinyrenderer")实现的cpu渲染

### 以下渲染原理的知识点

### 怎样画一条直线

#### Bresenham’s Line Drawing Algorithm

是一个经典的  **栅格化直线绘制算法** ，用于在像素网格中高效地绘制一条近似直线。它以  **整数运算为主** ，效率很高，广泛用于图形学与嵌入式图形显示。

//TODO图解

### 怎样绘制一个三角面：三角形光栅化

#### Old-school method: Line sweeping

是在现代光栅化技术（如 barycentric 插值、GPU 流程）出现前，**用扫描线逐行填充三角形的方法**

#### 原理概述：Line Sweeping Rasterization

给定一个三角形三个顶点 `v0(x0, y0)`、`v1(x1, y1)`、`v2(x2, y2)`，我们按照如下步骤执行：

##### 1. 按 y 坐标排序顶点：

//TODO 图解

此时：

* `v0` 是最上面的点
* `v1` 是中间点
* `v2` 是最下面的点

##### 2. 将三角形分成两个部分：

* **上半部分** ：v0 到 v1
* **下半部分** ：v1 到 v2

每一部分可以看作是两个顶点间的一条扫描线逐步水平填充。

//TODO图解
