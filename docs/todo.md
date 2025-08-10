# Framework TODO - Suggestions d'améliorations

## Philosophie
Garder le framework **KISS** (Keep It Simple and Stupid) pour permettre le développement rapide d'applications IoT variées.

## 🎯 Priorité HAUTE - Fonctionnalités essentielles

### 1. ScheduleManager - Programmation horaire
- [ ] Créer un manager pour gérer des programmations horaires
- [ ] Support des événements récurrents (quotidien, hebdomadaire, mensuel)
- [ ] Intégration avec NTP pour synchronisation horaire
- [ ] Persistance des programmes en SPIFFS/LittleFS
```cpp
// Exemple d'utilisation
schedule.add("arrosage_matin", "06:30", DAILY, []() { 
    deviceManager.getDevice("pompe")->setState(true);
});
schedule.add("volets_soir", "sunset-30min", DAILY, []() {
    deviceManager.getDevice("volets")->setState(false); 
});
```

### 2. RuleEngine - Moteur de règles simples
- [ ] Système de règles IF-THEN-ELSE configurable
- [ ] Support des conditions multiples (AND/OR)
- [ ] Actions déclenchées par événements ou seuils
- [ ] Configuration via JSON ou commandes MQTT
```cpp
// Exemple: VMC selon humidité
rule.create("vmc_auto")
    ->when("sensor/humidity", ">", 70)
    ->then("device/vmc", "on")
    ->otherwise("device/vmc", "off");
```

### 3. SensorManager - Gestion unifiée des capteurs
- [ ] Interface commune pour tous types de capteurs
- [ ] Moyennage et filtrage des valeurs
- [ ] Détection d'anomalies (valeurs hors plage)
- [ ] Publication automatique MQTT à intervalles configurables
```cpp
sensorManager.register(new DHT22Sensor(GPIO_4, "temperature"));
sensorManager.register(new PressureSensor(GPIO_ADC, "pression"));
sensorManager.setPublishInterval("temperature", 60000); // 1 min
```

### 4. PersistenceManager - Stockage local
- [ ] Sauvegarde/restauration état des devices
- [ ] Historique des valeurs capteurs (buffer circulaire)
- [ ] Configuration persistante des règles et programmes
- [ ] Gestion automatique de l'espace disponible

## 🔧 Priorité MOYENNE - Améliorations pratiques

### 5. WebUIManager - Interface web embarquée
- [ ] Dashboard simple et responsive
- [ ] Configuration WiFi/MQTT via web
- [ ] Visualisation état devices et capteurs
- [ ] Contrôle manuel des devices
- [ ] Génération automatique depuis configuration

### 6. AutoDiscoveryManager - Découverte automatique
- [ ] Support Home Assistant MQTT Discovery
- [ ] Publication automatique des capacités du device
- [ ] Configuration zéro pour intégration domotique
```cpp
autoDiscovery.enable("homeassistant");
autoDiscovery.publishDevice("projecteur_jardin", "light");
```

### 7. StateManager - Gestion d'états complexes
- [ ] Machine à états pour devices complexes
- [ ] Transitions avec conditions et timeouts
- [ ] Support des états composés (volet: ouvert/fermé/ouverture/fermeture)
```cpp
stateMachine.addState("idle")
    ->onEnter([]() { motor.stop(); })
    ->transition("opening", "cmd_open")
    ->transition("closing", "cmd_close");
```

### 8. NotificationManager - Alertes et notifications
- [ ] Envoi d'alertes via MQTT sur événements critiques
- [ ] Support des niveaux (info, warning, error, critical)
- [ ] Limitation du taux d'envoi (anti-spam)
- [ ] Templates de messages configurables

## 💡 Priorité BASSE - Nice to have

### 9. SceneManager - Gestion de scènes
- [ ] Groupes d'actions prédéfinies
- [ ] Activation par commande ou programme
- [ ] Support des transitions progressives
```cpp
scene.create("soiree")
    ->setDevice("lumiere_salon", 50)  // 50% intensité
    ->setDevice("volets", "closed")
    ->setDevice("musique", "on");
```

### 10. EnergyManager - Suivi consommation
- [ ] Monitoring consommation électrique
- [ ] Calcul coûts selon tarification
- [ ] Optimisation selon heures creuses/pleines
- [ ] Rapport périodique via MQTT

### 11. WeatherManager - Intégration météo
- [ ] Récupération données météo via API
- [ ] Utilisation dans règles (ex: arrosage selon pluie)
- [ ] Cache local des prévisions
```cpp
if (weather.getRainProbability() < 30) {
    schedule.enable("arrosage");
}
```

### 12. BackupManager - Sauvegarde/restauration
- [ ] Export configuration complète via MQTT
- [ ] Import configuration depuis MQTT
- [ ] Backup automatique périodique
- [ ] Versionning des configurations

## 📝 Améliorations code existant

### Optimisations
- [ ] Réduire l'utilisation mémoire des String (préférer char[])
- [ ] Pool de buffers réutilisables pour MQTT
- [ ] Lazy loading des managers non essentiels
- [ ] Mode deep sleep pour économie batterie

### Robustesse
- [ ] Watchdog timer pour auto-reset si blocage
- [ ] Gestion reconnexion WiFi/MQTT plus intelligente
- [ ] File d'attente MQTT avec retry en cas d'échec
- [ ] Validation entrées utilisateur systématique

### Documentation
- [ ] Exemples complets pour chaque cas d'usage
- [ ] Guide de démarrage rapide par type de projet
- [ ] Schémas de câblage types
- [ ] Bibliothèque de devices prêts à l'emploi

## 🚀 Exemples de projets types

### Projecteur automatique
```cpp
// Configuration simple
deviceManager.register(new RelayDevice("projecteur", GPIO_5));
sensorManager.register(new PIRSensor(GPIO_4, "presence"));
rule.create("auto_light")
    ->when("presence", "==", true)
    ->andWhen("time", "between", "sunset,sunrise")
    ->then("projecteur", "on", 300); // 5 min
```

### Gestion arrosage multi-zones
```cpp
// Zones d'arrosage
for (int i = 1; i <= 4; i++) {
    deviceManager.register(new ValveDevice("zone" + String(i), GPIO[i]));
}
// Programme automatique
schedule.add("arrosage_zones", "06:00", DAILY, []() {
    for (int i = 1; i <= 4; i++) {
        deviceManager.getDevice("zone" + String(i))->setState(true);
        delay(15 * 60 * 1000); // 15 min par zone
        deviceManager.getDevice("zone" + String(i))->setState(false);
    }
});
```

### Pompe forage avec protection pression
```cpp
sensorManager.register(new PressureSensor(GPIO_ADC, "pression"));
deviceManager.register(new RelayDevice("pompe", GPIO_5));
rule.create("protection_pompe")
    ->when("pression", "<", 1.5)  // bar
    ->then("pompe", "off")
    ->alert("Pression basse - pompe arrêtée");
```

### VMC intelligente
```cpp
sensorManager.register(new DHT22Sensor(GPIO_4, "humidity"));
deviceManager.register(new RelayDevice("vmc", GPIO_5));
rule.create("vmc_auto")
    ->when("humidity", ">", 65)
    ->orWhen("manual_mode", "==", true)
    ->then("vmc", "on")
    ->otherwise("vmc", "off");
```

### Filtration piscine adaptative
```cpp
sensorManager.register(new DS18B20Sensor(GPIO_4, "water_temp"));
rule.create("filtration_ete")
    ->when("month", "between", "5,9")  // Mai à Sept
    ->andWhen("water_temp", ">", 24)
    ->then([]() {
        schedule.update("filtration", "08:00", 8); // 8h/jour
    });
rule.create("filtration_hiver")
    ->when("month", "between", "11,2")  // Nov à Fév
    ->then([]() {
        schedule.update("filtration", "10:00", 4); // 4h/jour
    });
```

## 🎨 Principes de conception

1. **Simplicité avant tout** - API intuitive, configuration minimale
2. **Modularité** - Chaque manager indépendant et optionnel
3. **Configuration par défaut** - Fonctionne sans configuration complexe
4. **Extensibilité** - Facile d'ajouter ses propres devices/capteurs
5. **Résilience** - Gestion d'erreurs robuste, recovery automatique

## 📅 Roadmap suggérée

**Phase 1 (v1.1)** - Fondations
- ScheduleManager
- RuleEngine basique
- SensorManager

**Phase 2 (v1.2)** - Utilisabilité
- WebUIManager simple
- PersistenceManager
- AutoDiscoveryManager

**Phase 3 (v1.3)** - Intelligence
- StateManager
- NotificationManager
- RuleEngine avancé

**Phase 4 (v2.0)** - Écosystème
- SceneManager
- WeatherManager
- EnergyManager
- BackupManager