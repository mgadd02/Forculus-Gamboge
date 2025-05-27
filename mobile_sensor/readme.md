build command


west build -b thingy52/nrf52832 apps/final_project/mobile_sensor -p

### Flash Mobile Node (Thingy52)
west build -b thingy52/nrf52832 mycode/apps/final_project/mobile_sensor -p
west flash

### Flash Base Node (nRF52840-DK)
west build -b nrf52840dk/nrf52840 mycode/apps/final_project/base_sensor -p
west flash --recover