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
    api.send("/get_state")

    api.send("/ping")

    api.send("/set_state=config")
    api.send("/get_state")

    api.send("/get_config=focus")
    api.send("/set_config=focus:800")
    api.send("/get_config=focus")
    api.send("/set_state=idle")

    api.close()

# %%
