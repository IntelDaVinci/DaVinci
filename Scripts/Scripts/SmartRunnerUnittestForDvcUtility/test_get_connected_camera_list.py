import logging

def test_get_com_port():
    import coverage

    cov = coverage.Coverage(data_suffix='get_connected_camera_list', config_file=True)
    cov.start()

    from dvcutility import Enviroment
    logging.info("testing Enviroment.get_connected_camera_list")
    camera_list = Enviroment.get_connected_camera_list()
    cov.stop()
    cov.save()
    logging.info( "camera_list: "+ ';'.join(camera_list))

