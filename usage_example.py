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
    api.send("/ping")

    api.send("/get_state")
    api.send("/set_state=config")
    api.send("/get_state")

    api.send("/get_config=focus")
    api.send("/set_config=focus:800")
    api.send("/get_config=focus")

    api.send("/get_config=gain")
    api.send("/set_config=gain:+10")
    api.send("/set_config=gain:-10")
    api.send("/set_config=gain:+20")
    api.send("/get_config=gain")

    api.send("/get_config=exposure")
    api.send("/set_config=exposure:100.5")

    api.send("/set_state=idle")

    api.send("/trigger")
    api.send("/get_frame")

    api.close()

# %%
