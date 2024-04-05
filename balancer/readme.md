# balancer

Реализуйте простой балансер.
 
 **Балансер** -- это сервис, который принимает соединение от **клиента** и 
перенаправляет его на "наименее загруженный" **бэкенд**. Под перенаправлением имеется в виду, что балансер 
читает данные от клиента и отдаёт их бэкенду, и наоборот. Под загруженностью в данной задаче будем 
понимать количество открытых соединений к бэкенду. Если наименее загруженных несколько,
то соединение идёт с самым раннем в списке бэкендов. 

Вам также нужно реализовать возможность
**обнаружения "мёртвых" бэкендов** (к которым не получается подсоединиться),
чтобы подключения распределялись равномерно между живыми бэкендами. Бэкенд считается мёртвым,
если к нему не удаётся подключиться в течении 100 мс. После этого балансер не будет
пытаться соединиться с этим бэкендом.

Наконец, необходимо реализовать возможность **переконфигурации** балансера на лету.
А именно, класс балансера имеет метод `SetBackends`,
при вызове которого новые подключения должны распределяться по новым бэкендам.
Старые подключения при этом не закрываются.