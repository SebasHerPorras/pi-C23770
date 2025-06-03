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

## Paso 2: Crear VLAN y asignar puertos

Entrar al modo privilegiado:

```bash
Switch> enable
```

Entrar a configuración global:

```bash
Switch# configure terminal
```

Crear la VLAN 350 con nombre G3E5:

```bash
Switch(config)# vlan 350
Switch(config-vlan)# name G3E5
Switch(config-vlan)# exit
```

Asignar puertos Fa0/13 al Fa0/24 a la VLAN 350:

```bash
Switch(config)# interface range fa0/13 - 24
Switch(config-if-range)# switchport mode access
Switch(config-if-range)# switchport access vlan 350
Switch(config-if-range)# exit
```

Guardar los cambios:

```bash
Switch(config)# end
Switch# write memory
```

Building configuration...
[OK]

## Paso 3: Configurar la IP de gestión (VLAN 350)

Entrar al modo de configuración de la interfaz VLAN 350:

```bash
Switch# configure terminal
Switch(config)# interface vlan 350
```

Asignar IP y máscara:

```bash
Switch(config-if)# ip address 172.16.123.80 255.255.255.240
```

Activar la interfaz:

```bash
Switch(config-if)# no shutdown
```

Salir y guardar cambios:

```bash
Switch(config-if)# exit
Switch(config)# end
Switch# write memory
```

## Paso 4: Verificar configuración

Ver puertos asignados a VLANs:

```bash
Switch# show vlan brief
```

Salida esperada:

```bash
VLAN Name                             Status    Ports
---- -------------------------------- --------- -------------------------------
1    default                          active    Fa0/1 - Fa0/12, Gi0/1, Gi0/2
350  G3E5                             active    Fa0/13 - Fa0/24
```

Ver IP configurada:

```bash
Switch# show running-config
```

Fragmento relevante:

```bash
interface Vlan350
 ip address 172.16.123.80 255.255.255.240
 no ip route-cache
```

**Notas**
La VLAN 1 no tiene IP asignada, lo cual es recomendable por seguridad.

Para acceso remoto (telnet/SSH), también se necesita configurar un gateway y habilitar VTY lines con login y password.

El rango de IPs válidas para esa subred /28 es:

**IPs válidas**: 172.16.123.81 – 172.16.123.94
**Broadcast**: 172.16.123.95
