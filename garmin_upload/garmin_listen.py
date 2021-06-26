import datetime
import paho.mqtt.client as mqtt
import paho.mqtt.publish as mqttPub
import json
import logging
import logging.config
import yaml
import requests

from fit import FitEncoder_Weight
from garmin import GarminConnect
from body_metrics import bodyMetrics


CONFIG = 'config.yml'
LOGGER_CONFIG = 'logger.yml'
LOGGER_NAME = 'garmin_connector'


# Set up global logger
with open(LOGGER_CONFIG, 'r') as fp:
    loggerConfig = yaml.safe_load(fp)
logging.config.dictConfig(loggerConfig)
global logger
logger = logging.getLogger(LOGGER_NAME)

# Read general config
global config
with open(CONFIG, 'r') as fp:
    config = yaml.safe_load(fp)


def main():
    # Prepare all MQTT stuff
    mqttClient = mqtt.Client()
    mqttClient.on_connect = mqtt_on_connect
    mqttClient.on_message = mqtt_on_message
    mqttClient.username_pw_set(
        config['mqtt']['username'], password=config['mqtt']['password'])
    mqttClient.enable_logger(logger=logger)
    mqttClient.connect(
        host=config['mqtt']['host'], 
        port=config['mqtt']['port'], 
        keepalive=60)

    # And start the MQTT loop!
    mqttClient.loop_forever()


def mqtt_on_connect(client, userdata, flags, rc):
    logger.info(f'Connected to MQTT broker, result code {rc}')
    client.subscribe(config['mqtt']['topic'])


def check_entry_already_processed(data):
    # https://stackoverflow.com/a/25851972
    def _ordered(obj):
        if isinstance(obj, dict):
            return sorted((k, _ordered(v)) for k, v in obj.items())
        if isinstance(obj, list):
            return sorted(_ordered(x) for x in obj)
        else:
            return obj
            
    try:
        with open(config['last_ts'], 'r+') as fp:
            try:
                last_measure = json.load(fp)
            except json.decoder.JSONDecodeError:
                last_measure = None

            if _ordered(last_measure) != _ordered(data):
                json.dump(data, fp)
                return False
            else:
                return True
    except FileNotFoundError:
        with open(config['last_ts'], 'w') as fp:
            json.dump(data, fp)
            return False


def mqtt_on_message(client, userdata, msg):
    logger.info(f'Received payload: {msg.payload}')
    try:
        data = json.loads(msg.payload)
    except json.decoder.JSONDecodeError:
        logger.error('Could not parse JSON, ignoring')
        return

    # Has this entry been processed before? (MQTT messages are retained)
    if check_entry_already_processed(data):
        logger.info('Data is stale, ignoring')
        return
    else:
        logger.info('Data looks fresh, keeping track of it and continuing')

    height = float(config['garmin']['height'])

    # Weight has to be within the 10kg-200kg range. Metrics calculation will
    # fail if not within that range. Probably a bogus measurement anyway. Just
    # move on.
    weight = float(data.get('Weight', 0))
    if not weight or not 10 < weight < 200:
        logger.error('No weight data or weight value bogus, ignoring')
        return

    # Compute metrics if both weight and impedance are present.
    # Weight is already checked, additional check for impedance.
    if data.get('Impedance'):
        metrics = bodyMetrics(
            weight=weight,
            height=height,
            age=float((datetime.date.today() -
                       config['garmin']['dateOfBirth']).days / 365.25),
            sex=config['garmin']['sex'],
            impedance=float(data['Impedance'])
        )

    dtFormat = '%Y %m %d %H %M %S'
    try:
        dt = datetime.datetime.strptime(data.get('Timestamp'), dtFormat)
    except (TypeError, ValueError):
        # Cannot parse timestamp. Assuming measurement was done just now.
        dt = datetime.datetime.now()

    # Prepare Garmin data object
    fit = FitEncoder_Weight()
    fit.write_file_info()
    fit.write_file_creator()
    fit.write_device_info(timestamp=dt)
    if metrics:
        fit.write_weight_scale(
            timestamp=dt,
            weight=metrics.weight,
            bmi=metrics.getBMI(),
            percent_fat=metrics.getFatPercentage(),
            percent_hydration=metrics.getWaterPercentage(),
            visceral_fat_mass=metrics.getVisceralFat(),
            bone_mass=metrics.getBoneMass(),
            muscle_mass=metrics.getMuscleMass(),
            basal_met=metrics.getBMR(),
        )
    else:
        fit.write_weight_scale(
            timestamp=dt,
            weight=metrics.weight,
            bmi=weight/(height/100)**2
        )
    fit.finish()

    garmin = GarminConnect()
    try:
        garminSession = garmin.login(
            config['garmin']['username'], config['garmin']['password'])
    except requests.exceptions.ConnectionError:
        logger.error('Could not connect to Garmin Connect')
        return

    req = garmin.upload_file(fit.getvalue(), garminSession)
    if req:
        logger.info('Upload to Garmin succeeded')
    else:
        logger.info('Upload to Garmin failed')


if __name__ == '__main__':
    main()
