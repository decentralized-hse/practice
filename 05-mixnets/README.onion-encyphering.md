# Onion encyphering
Condition: onion encyphering so a message looks different at every step (with a static message one can match different steps)

i - the i-th node in the path from the receiver to the sender.

Algorithm:
1. The sender encrypts the message with keys A[i-1], where i goes down to 0. In other words, the message is encrypted first with A[0], then A[1], and so on, until A[i-1]. At each encryption step, an initialization vector (iv) is added to decrypt the message. This is crucial to ensure that nodes cannot determine how the message looked in previous steps;
2. The sender sends the encrypted message to the node;
3. Nodes through which the message will pass decrypt the message using the key A[i];
4. The receiver receives the message and decrypts it with the key A[0].