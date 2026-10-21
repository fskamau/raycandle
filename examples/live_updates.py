"""Template for endpoint-driven ticks; replace get_tick with your feed."""
# Assume df has columns ['o','h','l','c','sma'] and datetime/epoch index.
#
# fig=Figure(2,1)
# candle=fig.ax[0].plot(Candle(df[['o','h','l','c']]))
# sma=fig.ax[1].plot(df['sma'],name='SMA')
# fig.show(block=False)
# for epoch, price, sma_value in get_ticks_from_your_feed():
#     new_bar=candle.update_tick(epoch,price)
#     if new_bar: sma.shift()
#     sma.set_data([sma_value],start=len(sma.data)-1)
# fig.wait_closed()
# fig.close()
