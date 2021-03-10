# Лабораторная работа 2

**Название:** Разработка драйверов блочных устройств

**Цель работы:** Получить знания и навыки разработки драйверов блочных устройств для операционной системы Linux.

## Описание функциональности драйвера
Разработанный драйвер при загрузке создает блочное устройство с тремя первичными разделами: /dev/lab2p1 резмером 10МБ, /dev/lab2p2 резмером 25МБ и /dev/lab2p3 резмером 15МБ. Общий объем созданного устройства - 50 МБ.

## Инструкция по сборке
Драйвер собирается как модуль ядра Linux. Для сборки требуется перейти в директорию `lab2/` и выполнить команду `make`

Результат: в директории `lab2/` появился kernel object файл: `lab2.ko`

## Инструкция пользователя
После успешной сборки следует загрузить полученный kernel object файл, выполнив команду `insmod lab2.ko`.
Проверку созданного блочного устройства и его разделов можно осуществить с помощью команды `fdisk -l /dev/lab2`:
```
Disk /dev/lab2: 50 MiB, 52428800 bytes, 102400 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x00000000

Device      Boot Start    End Sectors Size Id Type
/dev/lab2p1          1  20479   20479  10M 83 Linux
/dev/lab2p2      20480  71679   51200  25M 83 Linux
/dev/lab2p3      71680 102399   30720  15M 83 Linux
```
## Примеры использования
После успешной загрузки модуля можно осуществить форматирование разделов созданного устройства и смонтировать их в директорию `/mnt`. Для этих целей был написан скрипт, который можно запустить с помощью команды `make setup`. Проверить результат работы скрипта можно с помощью команды `lsblk -l /dev/lab2`.
```
NAME   MAJ:MIN RM SIZE RO TYPE MOUNTPOINT
lab2   252:0    0  50M  0 disk
lab2p1 252:1    0  10M  0 part /mnt/lab2p1
lab2p2 252:2    0  25M  0 part /mnt/lab2p2
lab2p3 252:3    0  15M  0 part /mnt/lab2p3
```
Для проведения бенчмарков, измеряющих скорость передачи файла с размером, указанным в переменной окружении `BENCH_FILE_SIZE`, между различными разделами созданного блочного устройства и реального SSD был разработан скрипт на языке Python. Для запуска скрипта следует воспользоваться командой `make bench`.
```
src=/mnt/lab2p1, dst=/mnt/lab2p2, median speed=444.0, units=MB/s
src=/mnt/lab2p1, dst=/mnt/lab2p3, median speed=418.0, units=MB/s
src=/mnt/lab2p1, dst=/home/gleb, median speed=175.0, units=MB/s
src=/mnt/lab2p2, dst=/mnt/lab2p1, median speed=420.0, units=MB/s
src=/mnt/lab2p2, dst=/mnt/lab2p3, median speed=379.0, units=MB/s
src=/mnt/lab2p2, dst=/home/gleb, median speed=180.0, units=MB/s
src=/mnt/lab2p3, dst=/mnt/lab2p1, median speed=409.0, units=MB/s
src=/mnt/lab2p3, dst=/mnt/lab2p2, median speed=448.0, units=MB/s
src=/mnt/lab2p3, dst=/home/gleb, median speed=206.0, units=MB/s
src=/home/gleb, dst=/mnt/lab2p1, median speed=424.0, units=MB/s
src=/home/gleb, dst=/mnt/lab2p2, median speed=417.0, units=MB/s
src=/home/gleb, dst=/mnt/lab2p3, median speed=308.0, units=MB/s
```
Скорость передачи данных в среднем составляет 400 MB/s, при этом явно просматривается снижение скорости до 180-200 MB/s при чтении файла из RAM в SSD, что подтверждает теорию о более низкой скорости операций чтения/записи SDD по сравнению с RAM.

После звершения работы с блочным устройством следует отмонтировать разделы устройства с помощью команды `make umount` и выгрузить модуль из ядра командой `rmmod lab2`.
