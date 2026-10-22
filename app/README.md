# Test minimal des moteurs DRV8844

L'application utilise Zephyr et la carte Nucleo-H723ZG. Elle configure les deux
moteurs à l'arrêt au démarrage et expose des commandes via le shell série :

```text
motor set 0.2 0.2
motor set -0.2 0.2
motor stop
```

Les vitesses sont comprises entre `-1.0` (marche arrière à pleine puissance) et
`1.0` (marche avant à pleine puissance). Commencez par une faible valeur et
surélevez le robot pendant les premiers essais.

## Brochage

Le dépôt ne contient pas encore les broches de la carte reliées au driver.
Créez `app/boards/nucleo_h723zg.overlay` à partir des broches réellement câblées.
Le nœud `zephyr,user` doit fournir les propriétés suivantes :

- `left-enable-pwms` et `right-enable-pwms` : PWM des entrées EN1 et EN2 ;
- `left-input-a-gpios`, `left-input-b-gpios`, `right-input-a-gpios` et
  `right-input-b-gpios` : entrées IN1 à IN4 ;
- `reset-gpios` : ligne RSTDRV, avec la polarité active basse correctement
  déclarée ;
- `fault-gpios` : ligne FAULTDRV, avec la polarité active basse correctement
  déclarée.

Exemple de structure, à compléter avec les contrôleurs, canaux, périodes, pins
et états pinctrl correspondant au câblage de la Nucleo :

```dts
/ {
    zephyr,user {
        left-enable-pwms = <&pwm_controller CHANNEL PWM_PERIOD PWM_FLAGS>;
        right-enable-pwms = <&pwm_controller CHANNEL PWM_PERIOD PWM_FLAGS>;
        left-input-a-gpios = <&gpio_controller PIN GPIO_ACTIVE_HIGH>;
        left-input-b-gpios = <&gpio_controller PIN GPIO_ACTIVE_HIGH>;
        right-input-a-gpios = <&gpio_controller PIN GPIO_ACTIVE_HIGH>;
        right-input-b-gpios = <&gpio_controller PIN GPIO_ACTIVE_HIGH>;
        reset-gpios = <&gpio_controller PIN GPIO_ACTIVE_LOW>;
        fault-gpios = <&gpio_controller PIN GPIO_ACTIVE_LOW>;
    };
};
```

Les noms en majuscules sont des espaces réservés, pas des valeurs DTS valides.
Configurez également les états pinctrl des sorties PWM selon le manuel de la
carte. Sans ces propriétés, l'application compile en mode sûr et signale que le
brochage doit être renseigné ; elle ne commande pas les moteurs.

Compilation depuis la racine du dépôt, après installation de Zephyr et de `west` :

```sh
west build -b nucleo_h723zg app
```
