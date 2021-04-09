# Лабораторная работа 3

**Название:** "Разработка драйверов сетевых устройств"

**Цель работы:** Получить знания и навыки разработки драйверов сетевых интерфейсов для операционной системы Linux.

## Описание функциональности драйвера

Разработанный драйвер создает виртуальный интерфейс `vni0` на основе родительского `enp0s3` и перехватывает UDP-датаграммы, отправленные родителю. Если содержимое датаграммы совпадает с заданным фильтром, разобранная датаграмма фиксируется в файле `/proc/var4`. Фильтр задается через отправку датаграммы с содержимым в формате `filter:<filter>`

## Инструкция по сборке

Драйвер собирается как модуль ядра Linux. Для сборки требуется перейти в директорию lab3/ и выполнить команду make

Результат: в директории lab3/ появился kernel object файл: lab3.ko

## Инструкция пользователя

Загрузка модуля в ядро осуществляется командой `insomd lab3.ko`

Результат:

```
# dmesg
...
[Apr 9 08:29] Module lab3 loaded
[  +0,000002] lab3: create link vni0
[  +0,000001] lab3: registered rx handler for enp0s3
```

Проверка создания виртуального интерфейса `vni0`:

```
# ip addr
...
2: enp0s3: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 08:00:27:95:eb:5d brd ff:ff:ff:ff:ff:ff
    inet 192.168.56.101/24 brd 192.168.56.255 scope global dynamic noprefixroute enp0s3
       valid_lft 687sec preferred_lft 687sec
    inet6 fe80::a917:b82f:d799:d7da/64 scope link noprefixroute 
       valid_lft forever preferred_lft forever
10: vni0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN group default qlen 1000
    link/ether 08:00:27:95:eb:5d brd ff:ff:ff:ff:ff:ff
```

Выгрузка модуля в ядро осуществляется командой `rmmod lab3.ko`

Результат:

```
# dmesg
...
[Apr 9 08:31] lab3: unregister rx handler for enp0s3
[  +0,000051] vni0: device closed
```

## Примеры использования

Отправка UDP-датаграммы до установки фильтра:
```
echo "without-filters" | nc -u 192.168.56.101 53 -w1
```
Результат:
```
# dmesg
...
[  +0,000016] lab3: filter = , data = without-filters
# cat /proc/var4
#
```
Отправка UDP-датаграммы c установкой фильтра и UDP-датаграмм для фильтрации:
```
echo "filter:new-filter" | nc -u 192.168.56.101 53 -w1
echo "new-filter" | nc -u 192.168.56.101 53 -w1
echo "not-filter" | nc -u 192.168.56.101 53 -w1
```
Результат:
```
# dmesg
...
[  +0,000002] lab3: filter = , data = filter:new-filter
[  +0,000013] lab3: New filter: new-filter
[  +0,000016] lab3: filter = new-filter
              , data = new-filter
[  +0,000006] lab3: Captured filtered UDP datagram. Data: new-filter
[  +0,000018] lab3: filter = new-filter
              , data = not-filter
# cat /proc/var4
Captured UDP datagram, saddr: 192.168.56.1
daddr: 192.168.56.101
Data length: 11. Data:
new-filter
```
