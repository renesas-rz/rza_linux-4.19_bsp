/****************************************************************************
 *  Copyright (C) TES Electronic Solutions GmbH
 *  Project : D/AVE 2D
 ****************************************************************************/

#ifndef TES_DAVE_IRQ_H_
#define TES_DAVE_IRQ_H_

int register_irq(struct dave2d_dev *dave);
void unregister_irq(struct dave2d_dev *dave);
ssize_t dave2d_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);

#endif /* TES_DAVE_IRQ_H_ */
