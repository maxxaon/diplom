class Channel:
    packet_volume = 0.020

    def __init__(self, speed):
        self.speed = speed  # kB/s

    def get_transfer_time(self, data_volume):
        return (data_volume + self.packet_volume) / self.speed


class PageAccess:
    def __init__(self, access_time, access_type, page_num):
        self.access_time = access_time
        self.access_type = access_type
        self.page_num = page_num


class AccessType:
    READ = 'read'
    WRITE = 'write'


class Simulator:
    def __init__(self, page_cnt, rw_access_history, channel):
        self.page_cnt = page_cnt
        self.rw_access_history = rw_access_history
        self.channel = channel
        self.pages_to_transfer = [i for i in range(page_cnt)]
        self.vm_location = 0
        self.cur_time = 0
        self.page_size = 4
        self.page_num_size = 0.004
        self.memory_delay = 0
        self.downtime = 0

    def _delay_memory_access(self, value):
        for i in range(len(self.rw_access_history)):
            self.rw_access_history[i].access_time += value

    def _add_memory_delay(self, value):
        self._delay_memory_access(value)
        self.memory_delay += value

    def _add_downtime(self, value):
        self._delay_memory_access(value)
        self.downtime += value

    def _get_accessed_pages(self, access_type):
        while len(self.rw_access_history) > 0 and \
                self.rw_access_history[0].access_time + \
                self.channel.get_transfer_time(self.page_num_size) < self.cur_time:
            cur_access = self.rw_access_history[0]
            self.rw_access_history.pop(0)
            if cur_access.access_type == access_type:
                yield cur_access

    def start(self):
        while len(self.pages_to_transfer) > 0:
            while len(self.pages_to_transfer) > 0:
                # page transfer
                self.cur_time += self.channel.get_transfer_time(self.page_size)
                self.pages_to_transfer.pop(0)

                # checking for page faults
                # TODO only for postcopy
                if self.vm_location == 1:
                    for access in self._get_accessed_pages(AccessType.READ):
                        if access.page_num in self.pages_to_transfer != -1:
                            # resolve page fault
                            cur_delay = self.channel.get_transfer_time(self.page_size)
                            self._add_memory_delay(cur_delay)
                            self.cur_time += cur_delay
                            self.pages_to_transfer.pop(self.pages_to_transfer.index(access.page_num))

            # update pages_to_transfer
            for access in self._get_accessed_pages(AccessType.WRITE):
                self.pages_to_transfer.append(access.page_num)

            # check whether we will be able to transfer all remaining data in less than 30 ms
            # TODO only for precopy
            cur_expected_downtime = self.channel.get_transfer_time(self.pages_to_transfer * self.page_size)
            if cur_expected_downtime < 0.03:
                self._add_downtime(cur_expected_downtime)
                self.pages_to_transfer.clear()

        return self.cur_time, self.memory_delay, self.downtime


