#pragma once

void load_config_group(rd_kafka_conf_t *conf, GKeyFile *key_file,
                       const char *group);