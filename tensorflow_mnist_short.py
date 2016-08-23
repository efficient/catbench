#!/usr/bin/python3

# /usr/local/lib/python3.5/dist-packages/tensorflow/models/image/mnist/convolutional.py

import tensorflow.models.image.mnist.convolutional as convolutional

if __name__ == '__main__':
  convolutional.WORK_DIRECTORY = 'tensorflow_mnist_data'
  convolutional.NUM_EPOCHS = 1
  
  convolutional.main()

