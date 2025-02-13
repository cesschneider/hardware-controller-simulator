# %%
import time
import requests

BASE_URL = "http://localhost:7001"


class API:
    def __init__(self, base_url):
        self.session = requests.Session()
        self.base_url = base_url

    def send(self, endpoint):
        route = f"{self.base_url}{endpoint}"

        t0 = time.time()
        response = self.session.get(route)
        t1 = time.time()

        print(f"{(t1-t0):.3f}s\t{route}")
        print(f"{response.status_code}\t{response.text}")

        return response.text

    def close(self):
        self.session.close()


if __name__ == "__main__":
    api = API(BASE_URL)

    api.send("/reset")
    api.send("/resett") # invalid command
    api.send("/ping")

    '''
    api.send("/set_state=idle")
    api.send("/trigger")
    api.send("/get_state")
    time.sleep(5) # waiting 5 seconds until response is available
    api.send("/get_frame")
    api.send("/reset")

    api.send("/get_state")
    api.send("/set_state=config")
    api.send("/set_state=capture") # invalid state value
    api.send("/get_state")
    '''

    '''
        The `photometric_mode` and `led_pattern` have some interdependencies:
        - `photometric_mode` can only be turned **on** *(1)* if `led_pattern:a` ***(ALL OFF)***.
        - `led_pattern` can only be changed when `photometric_mode:0` ***(off)***.
        - `led_pattern` options:
            - `a`: *(ALL OFF)*
            - `b`: *(ALL ON)*
            - `c`: *(LEFT)*
            - `d`: *(RIGHT)*
            - `e`: *(LEFT + RIGHT)*
            - `f`: *(TOP)*
            - `g`: *(BOTTOM)*
            - `h`: *(ALL ONTOP + BOTTOM)*

        - Prevent the **Controller** from performing illegal operations:
          - Ex.: If the **Hardware** is on `photometric_mode:1` and the **Controller** sends 
          `set_config=led_pattern:b` to it, the **Driver** should not forward this to the 
          **Hardware** and return an error to the **Controller**.
    '''

    api.send("/get_state")
    api.send("/get_config=photometric_mode") # must return error as state is not set to config

    api.send("/set_state=config")
    api.send("/set_config=photometric_mode:1")
    api.send("/set_config=led_pattern:b") # must return an error, invalid operation

    api.send("/set_state=config")
    api.send("/set_config=photometric_mode:0")
    api.send("/set_config=led_pattern:b") # must accept new value

    '''
    api.send("/get_config=focus")
    api.send("/set_config=focus:800")
    api.send("/set_config=focus:1800") # invalid
    api.send("/get_config=focus")

    api.send("/get_config=gain")
    api.send("/set_config=gain:+10")
    api.send("/set_config=gain:-10")
    api.send("/set_config=gain:+20") # invalid
    api.send("/get_config=gain")

    api.send("/get_config=exposure")
    api.send("/set_config=exposure:100.5")
    api.send("/set_config=exposure:1000.5") # invalid

    api.send("/set_config=photometric_mode:2") # invalid
    api.send("/set_config=photometric_mode:0")
    api.send("/get_config=photometric_mode")

    api.send("/trigger") # invalid operation
    api.send("/set_state=idle")
    api.send("/trigger")

    api.send("/get_state")
    api.send("/trigger") # invalid operation

    time.sleep(5) # waiting 5 seconds until response is available
    api.send("/get_frame")
    api.send("/reset")

    api.send("/set_state=config")
    api.send("/set_config=photometric_mode:1")
    api.send("/get_config=photometric_mode")

    api.send("/set_state=idle")
    api.send("/trigger")
    api.send("/get_frame") # this should return an error because didn't wait enough time

    time.sleep(5) # waiting 5 seconds until response is available
    api.send("/get_frame")

    time.sleep(5) # waiting 5 seconds until response is available
    api.send("/get_frame")

    time.sleep(5) # waiting 5 seconds until response is available
    api.send("/get_frame")
    '''

    api.close()

# %%
