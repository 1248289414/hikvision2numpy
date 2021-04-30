import numpy as np

import hikvisionpy
import cv2
import time

hikvisionpy.init('192.168.1.64', 'admin', 'a123456789')
time.sleep(1)
height = hikvisionpy.getHeight()
width = hikvisionpy.getWidth()
print([height, width, 3])
frame = np.empty([height, width, 3], dtype='uint8') # 需要先给frame分配内存

while True:
    hikvisionpy.getFrame(frame)
    cv2.imshow('view',frame)
    if cv2.waitKey(1) == 27: #esc
        break

hikvisionpy.release()