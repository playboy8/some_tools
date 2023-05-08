import sys
import numpy as np
import pandas as pd
import pyqtgraph as pg
from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QLabel, QHBoxLayout, QVBoxLayout
from PyQt5.QtGui import QPicture, QPainter
from PyQt5.QtCore import Qt, QRectF
import tushare as ts

class KLineItem(pg.GraphicsObject):
    def __init__(self, data):
        super().__init__()
        self.data = data  # data是一个DataFrame，包含了开盘价、收盘价、最高价、最低价、成交量、KDJ等数据
        self.picture = QPicture()  # 用于缓存绘图对象
        self.generatePicture()  # 调用自定义的绘图方法

    def generatePicture(self):
        p = QPainter(self.picture)  # 创建绘图对象
        p.setPen(pg.mkPen('w'))  # 设置画笔颜色为白色
        w = (self.data.index[1] - self.data.index[0]) * 0.3  # 设置蜡烛图的宽度为两个数据点之间距离的30%
        for (t, open, close, high, low) in zip(self.data.index, self.data['open'], self.data['close'], self.data['high'], self.data['low']):
            if open > close:  # 如果开盘价大于收盘价，说明是阴线，用红色表示
                p.setBrush(pg.mkBrush('r'))
            else:  # 否则是阳线，用绿色表示
                p.setBrush(pg.mkBrush('g'))
            p.drawLine(QPointF(t, low), QPointF(t, high))  # 绘制最高价和最低价之间的竖线
            p.drawRect(QRectF(t-w, open, w*2, close-open))  # 绘制开盘价和收盘价之间的矩形
        p.setPen(pg.mkPen('y'))  # 设置画笔颜色为黄色
        p.setBrush(pg.mkBrush('y'))  # 设置画刷颜色为黄色
        for (t, ma) in zip(self.data.index, self.data['ma']):  # 绘制移动平均线的数据点
            p.drawEllipse(QPointF(t, ma), w*0.3, w*0.3)  # 绘制圆形点
        p.setPen(pg.mkPen('c'))  # 设置画笔颜色为青色
        p.setBrush(pg.mkBrush('c'))  # 设置画刷颜色为青色
        for (t, vol) in zip(self.data.index, self.data['volume']):  # 绘制成交量的柱状图
            p.drawRect(QRectF(t-w*0.8, 0, w*1.6, vol))  # 绘制矩形柱子
        p.setPen(pg.mkPen('m'))  # 设置画笔颜色为洋红色
        for (t1, k1), (t2, k2) in zip(self.data[:-1].iterrows(), self.data[1:].iterrows()):  # 绘制KDJ指标中的K线
            p.drawLine(QPointF(t1[0], k1[1]['k']), QPointF(t2[0], k2[1]['k']))  # 绘制两个数据点之间的连线
        p.setPen(pg.mkPen('y'))  # 设置画笔颜色为黄色
        for (t1, d1), (t2, d2) in zip(self.data[:-1].iterrows(), self.data[1:].iterrows()):  # 绘制KDJ指标中的D线
            p.drawLine(QPointF(t1[0], d1[1]['d']), QPointF(t2[0], d2[1]['d']))  # 绘制两个数据点之间的连线
        p.setPen(pg.mkPen('c'))  # 设置画笔颜色为青色
        for (t1, j1), (t2, j2) in zip(self.data[:-1].iterrows(), self.data[1:].iterrows()):  # 绘制KDJ指标中的J线
            p.drawLine(QPointF(t1[0], j1[1]['j']), QPointF(t2[0], j2[1]['j']))  # 绘制两个数据点之间的连线
        p.end()  # 结束绘图

    def paint(self, p, *args):  # 实现继承自父类的paint方法，将缓存的绘图对象绘制到屏幕上
        p.drawPicture(0, 0, self.picture)

    def boundingRect(self):  # 实现继承自父类的boundingRect方法，返回绘图对象的边界矩形，用于控制视图范围
        return QRectF(self.picture.boundingRect())

class KLineWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.initUI()  # 调用自定义的初始化界面方法

    def initUI(self):
        self.setWindowTitle("K线图")  # 设置窗口标题

        self.token = 'bc462a4181ec6cd18f6983adce52e99841f648450f2cc38c549529b1'  # tushare账户token
        ts.set_token(self.token)   # 设置token
        self.pro = ts.pro_api()   # 创建pro接口对象
        self.stock_code = '600519.SH'   # 默认股票代码
        self.period = 'D'   # 默认周期
        self.start_date = '20200101'   # 默认开始日期
        self.end_date = '20211231'   # 默认结束日期
        self.df = None   # 初始化数据框为空
        self.kline = None   # 初始化K线图对象为空
        self.label_code = QLabel("股票代码：")   # 创建显示股票代码的标签
        self.label_period = QLabel("周期：")   # 创建显示周期的标签
        self.label_start = QLabel("开始日期：")   # 创建显示开始日期的标签
        self.label_end = QLabel("结束日期：")   # 创建显示结束日期的标签
        self.button_day = QPushButton("日线")   # 创建切换日线周期的按钮
        self.button_week = QPushButton("周线")   # 创建切换周线周期的按钮
        self.button_month = QPushButton("月线")   # 创建切换月线周期的按钮
        self.button_pre = QPushButton("<")   # 创建切换上一只股票的按钮
        self.button_next = QPushButton(">")   # 创建切换下一只股票的按钮
        self.button_update = QPushButton("更新")   # 创建更新数据和绘图的按钮
        self.label_ma = QLabel()   # 创建显示移动平均线数据的标签
        self.label_vol = QLabel()   # 创建显示成交量数据的标签
        self.label_kdj = QLabel()   # 创建显示KDJ指标数据的标签
        self.label_price = QLabel()   # 创建显示当前股票价格信息的标签

        self.label_date = QLabel()   # 创建显示当前日期信息的标签
        
        layout_top = QHBoxLayout()
        layout_top.addWidget(self.label_code)
        layout_top.addWidget(self.button_pre)
        layout_top.addWidget(self.button_next)
        layout_top.addWidget(self.label_period)
        layout_top.addWidget(self.button_day)
        layout_top.addWidget(self.button_week)
        layout_top.addWidget(self.button_month)
        layout_top.addWidget(self.label_start)
        layout_top.addWidget(self.label_end)
        layout_top.addWidget(self.button_update)

        layout_bottom_left_1 = QHBoxLayout()
        layout_bottom_left_1.addWidget(self.label_ma)

        layout_bottom_left_2 = QHBoxLayout()
        layout_bottom_left_2.addWidget(self.label_vol)

        layout_bottom_left_3 = QHBoxLayout()
        layout_bottom_left_3.addWidget(self.label_kdj)

        layout_bottom_left_vbox = QVBoxLayout()
        layout_bottom_left_vbox.addLayout(layout_bottom_left_1)
        layout_bottom_left_vbox.addLayout(layout_bottom_left_2)
        layout_bottom_left_vbox.addLayout(layout_bottom_left_3)

        layout_bottom_right_hbox = QHBoxLayout()
        layout_bottom_right_hbox.addWidget(self.label_price)
        layout_bottom_right_hbox.addWidget(self.label_date)

        layout_bottom_hbox = QHBoxLayout()
        layout_bottom_hbox.addLayout(layout_bottom_left_vbox)
        layout_bottom_hbox.addLayout(layout_bottom_right_hbox)

        layout_main_vbox = QVBoxLayout()

        layout_main_vbox.addLayout(layout_top)
        self.kline_widget = pg.PlotWidget()  # 创建K线图控件
        layout_main_vbox.addWidget(self.kline_widget)  # 添加K线图控件
        layout_main_vbox.addLayout(layout_bottom_hbox)

        self.setLayout(layout_main_vbox)  # 设置主窗口布局

        self.button_update.clicked.connect(self.update_data_and_plot)  # 连接更新数据和绘图的槽函数

    def update_data_and_plot(self):  # 更新数据和绘图的槽函数
        self.df = self.pro.daily(ts_code=self.stock_code, start_date=self.start_date, end_date=self.end_date)  # 获取日线数据
        if self.period == 'W':  # 如果周期是周线
            self.df = self.df.resample('W', on='trade_date').last()  # 对日线数据进行周采样
        elif self.period == 'M':  # 如果周期是月线
            self.df = self.df.resample('M', on='trade_date').last()  # 对日线数据进行月采样
        self.df['ma'] = self.df['close'].rolling(5).mean()  # 计算5日移动平均线
        low_list = self.df['low'].rolling(9, min_periods=1).min()  # 计算最近9日的最低价
        high_list = self.df['high'].rolling(9, min_periods=1).max()  # 计算最近9日的最高价
        rsv = (self.df['close'] - low_list) / (high_list - low_list) * 100  # 计算未成熟随机值
        self.df['k'] = rsv.ewm(com=2, adjust=False).mean()  # 计算K值
        self.df['d'] = self.df['k'].ewm(com=2, adjust=False).mean()  # 计算D值
        self.df['j'] = 3 * self.df['k'] - 2 * self.df['d']  # 计算J值
        self.kline = KLineItem(self.df)  # 创建K线图对象
        self.kline_widget.clear()  # 清空K线图控件中的内容
        self.kline_widget.addItem(self.kline)  # 添加K线图对象到控件中
        self.kline.setXRange(0, len(self.df))  # 设置横轴范围为数据的长度
        self.kline.setYRange(self.df['low'].min(), self.df['high'].max())  # 设置纵轴范围为最低价和最高价之间

if __name__ == '__main__':
    app = QApplication(sys.argv)  # 创建应用程序对象
    window = KLineWidget()  # 创建主窗口对象
    window.show()  # 显示主窗口
    sys.exit(app.exec_())  # 进入事件循环

