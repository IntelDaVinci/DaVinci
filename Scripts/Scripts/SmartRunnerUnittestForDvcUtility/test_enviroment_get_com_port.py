import logging

def test_get_com_port():
    import coverage

    cov = coverage.Coverage(data_suffix='get_com_port', config_file=True)
    cov.start()

    from dvcutility import Enviroment
    logging.info("testing Enviroment.get_com_port")
    port_list = Enviroment.get_com_port()
    cov.stop()
    cov.save()
    logging.info( "port_list: "+ ';'.join(port_list))

