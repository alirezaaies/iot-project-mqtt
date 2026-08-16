# Backend

Backend means the main Python part of the system that runs behind the web pages and devices.

Put these items here:

- MQTT connection and message receiving;
- message validation and conversion;
- main operations such as recording a sensor reading or sending a command;
- coordination between the web application and database functions;
- logging and error handling related to these operations.

Do not put HTML, CSS, Flask routes, Arduino code, or direct SQL files here.

Start with a few clearly named Python files in this directory. Split them into subdirectories only if this folder becomes difficult to navigate.
