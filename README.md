## TinyHttpServer



#### 使用和配置

#### MySQL建表

	mysql> create database sqlname;
	mysql> use sqlname;
	mysql> create table user(username char(50) NULL, passwd char(50) NULL)ENGINE=InnoDB;

#### 修改对应的数据

	在main.cpp的config函数中将 user 、 passwd 、 databasename 都更改成mysql中的设置。

#### 安装程序

	$git clone 项目地址
	$cd webserver/
	$make


