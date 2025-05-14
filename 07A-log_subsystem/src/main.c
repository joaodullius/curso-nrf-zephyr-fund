#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <string.h>

LOG_MODULE_REGISTER(log_subsystem, LOG_LEVEL_DBG);

void simulate_logging(void)
{
    
}

int main(void)
{
	LOG_INF("Mensagem informativa de status.");
    LOG_WRN("Aviso detectado em operação.");
    LOG_ERR("Erro simulado no subsistema!");
    LOG_DBG("Mensagem de debug (detalhes internos).");

    const char buffer[] = "Mensagem binária";
    LOG_HEXDUMP_INF(buffer, strlen(buffer), "Dump de memória");
}
