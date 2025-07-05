# Manual de Configuración: Switch Cisco Catalyst 2960 – Isla 5 (VLAN 350)

## Paso 1: Borrar la configuración anterior y reiniciar el switch

Entrar al modo privilegiado:

```bash
Switch> enable
```

**Borrar la configuración guardada (startup-config) y VLANs anteriores:**

```bash
Switch# delete flash:config.text
```

Delete filename [config.text]?  [ENTER]  
%Error deleting flash:config.text (No such file or directory)   <-- Si no existe, es normal.

```bash
Switch# delete flash:vlan.dat
```

Delete filename [vlan.dat]?  [ENTER]  
Delete flash:vlan.dat? [confirm]  [ENTER]

Reiniciar el switch:

```bash
Switch# reload
```

Proceed with reload? [confirm]  [ENTER]  
Al reiniciar, salir del diálogo de configuración inicial:

Would you like to enter the initial configuration dialog? [yes/no]: no

# Configuración Física - Isla 5 (VLAN 350)

Este documento sirve como guía para configurar toda la infraestructura física correspondiente a la **Isla 5** utilizando **VLAN 350**, incluyendo:

- Direccionamiento IP de las PCs
- Configuración del Switch L2 (2960)
- Configuración relevante del Switch L3 (3560)

---

## 1. Direccionamiento IP para PCs (Isla 5)

Asignar estáticamente estas direcciones IP a cada PC:

| PC  | IP              | Máscara             | Gateway           |
|-----|------------------|----------------------|--------------------|
| PC1 | 172.16.123.83   | 255.255.255.240      | 172.16.123.81     |
| PC2 | 172.16.123.84   | 255.255.255.240      | 172.16.123.81     |
| PC3 | 172.16.123.85   | 255.255.255.240      | 172.16.123.81     |
| PC4 | 172.16.123.86   | 255.255.255.240      | 172.16.123.81     |

---

## 2. Configuración del Switch L2 (2960) - Isla 5

```bash
enable
configure terminal

#Crear VLAN 350
vlan 350
 name ISLA_5
exit

#Asignar puertos de acceso para PCs
interface range fa0/13 - 16
 switchport mode access
 switchport access vlan 350
exit

#Configurar puerto troncal hacia el Switch L3
interface fa0/24
 switchport mode trunk
exit

#Configurar interfaz VLAN para gestión
interface vlan 350
 ip address 172.16.123.82 255.255.255.240
 no shutdown
exit

#Establecer puerta de enlace por defecto
ip default-gateway 172.16.123.81

end
write memory
```

---

## 3. Fragmento relevante en el Switch L3 (3560)

Asegúrese de que el Switch L3 tiene la siguiente configuración para VLAN 350:

```bash
enable
configure terminal

ip routing

#Crear VLAN 350
vlan 350
 name ISLA_5
exit

#Interfaz VLAN con IP como gateway
interface vlan 350
 ip address 172.16.123.81 255.255.255.240
 no shutdown
exit

#Configurar puerto troncal hacia el Switch L2 (Isla 5)
interface fa0/5
 switchport trunk encapsulation dot1q
 switchport mode trunk
exit

end
write memory
```

---

## 4. Verificación (desde PCs)

Ejecutar desde cada PC:

```bash
ping 172.16.123.81     # Switch L3
ping 172.16.123.82     # Switch L2
ping 172.16.123.84     # Otra PC en la isla, .85 .86 también
```

Todo debe responder correctamente si la configuración es exitosa.

---

**Nota:** Esta guía cubre exclusivamente la parte de la red física en Isla 5. Para conectividad entre islas, se debe asegurar que el switch de capa 3 tenga configuradas las otras VLANs también.
