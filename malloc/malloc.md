# TP2: Manejo del heap (librería malloc)

### Diseño librería malloc

Decidimos no implementar una estructura bloque en sí misma, sino que lo que tenemos son 3 arreglos de regiones, 
uno para cada tipo de bloque (pequeño, mediano y grande). Cada posición del arreglo contiene la primera región
de un bloque, así podemos identificar el inicio de cada bloque. 
Las regiones tienen estructura de lista doblemente enlazada, para poder obtener las regiones anteriores y 
posteriores. Además, contienen el tamaño que esutilizable y un booleano para indicar si la región está libre o no.
Tenemos un enum que define el tipo de dato block_size_t con los tres tipos de bloques, con los tamaños indicados
en el enunciado del trabajo práctico.
Por sugerencia de Dato implementamos un struct arena que agrupa los arreglos de bloques con su respectivo tipo
de bloque. Entonces tenemos arenas pequeña, mediana y grande que contienen el block_size_t y el arreglo con los
bloques para cada tipo. Así se evita tener por separado los block_size_t y los arreglos como variables globales, 
sino que se agrupan en el struct, y además se limpia el código en create_block y delete_block.

---

### Estructura de la librería

Decidimos dejar en los archivos malloc únicamente las funciones de la API pública de la librería:
malloc, calloc, realloc y free.
En los archivos block se encuentran las funciones y estructuras que implementan la lógica de los bloques y
las regiones. Estas funciones internas son las que utilizan las funciones de la API pública de malloc.

---

### Estrategias de búsqueda de reginoes libres

Para la búsqueda de regiones libres implementamos las estrategias de first fit y best fit a nivel bloques.
Esto quiere decir que se aplica cada algoritmo a los arreglos de los tres tipos de bloques, intentando buscar
primero en los bloques más pequeños posibles para la memoria pedida.
Por ejemplo, para un pedido de 20000 bytes que no entra en un bloque pequeño, se aplica directamente first fit
o best fit en el arreglo de bloques medianos. Si no se encuentra un bloque allí, se aplica el algoritmo en el
siguiente arreglo, en este caso el arreglo de bloques grandes.

---

### Tamaño máximo de memoria

No definimos una constante que determine el tamaño máximo de memoria administrado por nuestra librería, sino
que queda definido por la constante MAX_BLOCKS, que es la cantidad máxima de cada tipo de bloque.
Como MAX_BLOCKS es 50, esto significa que cada arreglo de bloques va a tener como máximo 50 bloques, y eso
va a definir el tamaño máximo de memoria.
Entonces tendremos 50 bloques pequeños, 50 medianos y 50 grandes.
Esto da un máximo de memoria de 1.730.969.600 bytes, es decir 1.7 GB.
Elegimos utilizar el mismo MAX_BLOCKS para los tres tipos de bloques ya que no sabemos para qué va a utilizar
la librería el usuario, si va a usar más bloques pequeños o grandes, entonces decidimos que haya la misma
cantidad máxima para los tres tipos de bloques.

---

### Tamaño mínimo de región

Está definido por la constante REGION_MIN_SIZE con valor de 256 bytes. Decidimos utilizar este valor para
evitar la fragmentación de la memoria, ya que si utilizamos regiones más chicas cada bloque se va a dividir
en muchas regiones muy pequeñas, lo que puede ser problemático en terminos de performance.

---

### Checksum y magic bytes

Para nuestras regiones implementamos un campo con un checksum, que adquiere el valor definido en la constante
MAGIC_BYTES con el valor 23072000 que se asigna cuando se crea una nueva región.
De esta forma podemos validar que los punteros que se utilicen en funciones como free o realloc sean
punteros generados por nuestra librería mediante un malloc previo.
Así se evitan errores por utilizar punteros inválidos.

---
