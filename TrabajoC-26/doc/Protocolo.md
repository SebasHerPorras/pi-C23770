# Protocolo de Red para Figuras en ASCII

El protocolo funciona en base a un llamado de tipo “GET” al server, es un modelo simple cuyas únicas funciones serán retornar mensajes de error en caso de que no siga el formato correcto o no encuentre lo que el cliente le solicitó y dos funcionalidades del llamado “GET”; pedir la lista de figuras disponibles y solicitar una figura en específico.

El protocolo funcionaría de la siguiente manera:

```cpp
int serverRequest(const std::string& request, std::string& response);
```

Donde el `request` será la acción que el cliente quiera consultar sobre el server y el `response` dónde vendrá la respuesta del servidor además del tipo de retorno de tipo entero que manejará los errores:  
- `0` → Éxito.  
- `-1` → Error.  

## Tipos de Solicitudes y Respuestas

### Obtener la lista de figuras disponibles
**Solicitud:**  
```
GET /figures
```
**Respuesta esperada:**  
```json
["ballena", "perro", "gato"]
```
**Código de retorno:**  
- `0` → Éxito.  
- `-1` → Error (si el formato es incorrecto).

---

### Obtener una figura en ASCII específica
**Solicitud:**  
```
GET /figures/{nombre_figura}
```
**Respuesta esperada:**  

Si la figura existe:
```json
{
  "name": "conejo",
  "ascii": "(\__/)\n(='.'=)\n(\")_(\")"
}
```
Si la figura no existe:
```json
{
  "error": "Figura no encontrada",
  "ascii": "figura de error"
}
```
**Código de retorno:**  
- `0` → Éxito.  
- `-1` → Error (si la figura no existe o la solicitud es incorrecta).

---

## Ejemplo de Uso
```cpp
std::string response;
int status = serverRequest("GET /figures", response);

if (status == 0) {
    status = serverRequest("GET /figures/conejo", response);
    std::cout << "Figura en ASCII:\n" << response << std::endl;
} else {
    std::cout << response << std::endl;
}
```

### Salida esperada:
```json
{
  "name": "conejo",
  "ascii": "(\__/)\n(='.'=)\n(\")_(\")"
}
```
o en caso de error:
```json
{
  "error": "Figura no encontrada",
  "ascii": "figura de error"
}
```
