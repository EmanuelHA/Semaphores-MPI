### a)
Es arquitecturalmente imposible que ocurran accidentes porque los procesos no comparten memoria [no hay una variable global (como en_cruce) que pueda sufrir RC's]. La exclusión mutua se logra mediante el paso de mensajes, donde el "coordinador" actúa como un semáforo centralizado que solo otorga un TAG_PERMISO a la vez y no emite el siguiente hasta recibir el TAG_CRUCE_FIN. Es por ello que no necesitamos código para detectar accidentes.  


### b)
Es más fácil razonar en MPI, ya que el estado del sistema está centralizado en el "coordinador" y la sincronización es explícita a través del flujo de mensajes, al contrario que en el teabajo anterior, donde cada hilo llega de manera impredecible al ser asignado por el sistema. Para escalar a 100 carriles la versión MPI es la ideal porque básicamente MPI está pensado para sistemas de memoria distribuida. Se podría ejecutar en múltiples ordenadores conectados por red, mientras que la versión anterior está limitada a los recursos de la máquina


### ​c)
Sí, el coordinador puede convertirse en un cuello de botella si la cantidad de solicitudes de los carriles excede su capacidad para procesar mensajes, saturando la red o la CPU del "coordinador". Un diseño sin coordinador central que garantice exclusión mutua podría implementarse usando un algoritmo de paso de "Token Ring" o similar, donde un único mensaje (token) circula entre los carriles en anillo. De este modo un carril solo puede cruzar si posee el token, garantizando que solo haya un vehículo cruzando a la vez.  


​### d)
La ventaja principal de MPI_ANY_SOURCE es que permite al "coordinador" atender al primer carril que esté listo, evitando bloqueos innecesarios. Sin embargo, puede causar starvation si la latencia de red de un nodo es de algún modo menor, permitiendo así que este "se adueñe" del cruce. En los datos del reporte, esto se detectaría si un carril muestra un "tiempo de espera acumulado" sospechosamente mayor que los demás. También analizando la tendencia al lugar que ocupa de todos los nodos al finalizar el cruce (ej. si siempre termina de último)
​

### e)
Al implementar MPI_Irecv en lugar de MPI_Recv, el coordinador no se bloquea cuando espera un mensaje. La ventaja es que permite al "coordinador" realizar otras tareas mientras el mensaje llega en segundo plano por la red. El orden de simulación no cambia, pero el uso del CPU del "coordinador", en este caso el rank 0 (RANK_MASTER) se vuelve más eficiente.
