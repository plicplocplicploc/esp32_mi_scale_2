import datetime
import json
import logging
import subprocess

import paho.mqtt.client as mqtt
import paho.mqtt.publish as mqttPub
import yaml

CONFIG = "config.yml"
CONFIG_MQTT = "/mnt/ssd/acc_creds/mqtt.json"
LOG_LEVEL = logging.INFO

DT_FORMAT_SRC = "%Y %m %d %H %M %S"
DT_FORMAT_DST = "%Y-%m-%d"


def prepare_logger(logger_name: str = __name__) -> logging.Logger:
    """Simple logger preparation function"""
    logger = logging.getLogger(logger_name)
    logger.setLevel(LOG_LEVEL)
    handler = logging.StreamHandler()
    formatter = logging.Formatter(
        "{asctime} {name} {levelname:8s} {message}", style="{"
    )
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    return logger


def main():
    mqttClient = mqtt.Client()
    mqttClient.on_connect = mqtt_on_connect
    mqttClient.on_message = mqtt_on_message
    mqttClient.username_pw_set(
        config["mqtt"]["username"], password=config["mqtt"]["password"]
    )
    mqttClient.enable_logger(logger=logger)
    mqttClient.connect(
        host=config["mqtt"]["host"], port=config["mqtt"]["port"], keepalive=60
    )

    mqttClient.loop_forever()


def mqtt_on_connect(client, userdata, flags, rc):
    logger.info(f"Connected to MQTT broker, result code {rc}")
    client.subscribe(config["mqtt"]["topic"])


def mqtt_on_message(client, userdata, msg):
    logger.info(f"Received payload: {msg.payload}")
    try:
        data = json.loads(msg.payload)
    except json.decoder.JSONDecodeError:
        logger.error("Could not parse JSON, ignoring")
        return

    # At this point, the payload is valid: let's ack
    mqttPub.single(
        config["mqtt"]["topic_ack"],
        payload=config["mqtt"]["ack_signal"],
        hostname=config["mqtt"]["host"],
        port=config["mqtt"]["port"],
        auth={
            "username": config["mqtt"]["username"],
            "password": config["mqtt"]["password"],
        },
    )

    # Append to full raw backup (mostly for debug)
    with open(config["full_raw_data"], "a") as fp:
        fp.write(str(data) + "\n")

    weight = float(data.get("Weight"))
    impedance = float(data.get("Impedance"))
    if not (weight and impedance):
        logger.error("Either weight or impedance missing in scale data")
        return

    date = (
        datetime.datetime.strptime(data["Timestamp"], DT_FORMAT_SRC)
        .date()
        .strftime(DT_FORMAT_DST)
    )

    # Post-weigh-in
    if config["post_weighin"]:
        logger.info("Starting post-update script")
        subprocess.run([config["post_weighin"], str(date), str(weight), str(impedance)])

    logger.info("All done!")


if __name__ == "__main__":
    with open(CONFIG, "r") as fp:
        config = yaml.safe_load(fp)
    with open(CONFIG_MQTT, "r") as fp:
        j = json.load(fp)
        config["mqtt"].update(j["mqtt"])

    logger = prepare_logger("mqtt_listen")

    main()
