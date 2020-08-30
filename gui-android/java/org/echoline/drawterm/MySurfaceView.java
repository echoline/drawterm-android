package org.echoline.drawterm;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import android.widget.CheckBox;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.security.spec.ECField;

/**
 * Created by eli on 12/3/17.
 */
public class MySurfaceView extends SurfaceView {
    private Bitmap bmp;
    private int screenWidth, screenHeight;
    private MainActivity mainActivity;
    private float ws, hs;
    private Paint paint = new Paint();

    public MySurfaceView(Context context, int w, int h, float ws, float hs) {
        super(context);
        screenHeight = h;
        screenWidth = w;
        this.ws = ws;
        this.hs = hs;
        mainActivity = (MainActivity)context;
        mainActivity.setWidth(screenWidth);
        mainActivity.setHeight(screenHeight);
        mainActivity.setWidthScale(ws);
        mainActivity.setHeightScale(hs);
        setWillNotDraw(false);

        Listener listener = new Listener();
        listener.onPrimaryClipChanged();
        ClipboardManager cm = (ClipboardManager)mainActivity.getApplicationContext().getSystemService(Context.CLIPBOARD_SERVICE);
        if (cm != null)
            cm.addPrimaryClipChangedListener(listener);

        setOnTouchListener(new View.OnTouchListener() {
            private int[] mouse = new int[3];

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                CheckBox left = (CheckBox)mainActivity.findViewById(R.id.mouseLeft);
                CheckBox middle = (CheckBox)mainActivity.findViewById(R.id.mouseMiddle);
                CheckBox right = (CheckBox)mainActivity.findViewById(R.id.mouseRight);
                CheckBox up = (CheckBox)mainActivity.findViewById(R.id.mouseUp);
                CheckBox down = (CheckBox)mainActivity.findViewById(R.id.mouseDown);
                int buttons = (left.isChecked()? 1: 0) |
                                (middle.isChecked()? 2: 0) |
                                (right.isChecked()? 4: 0) |
                                (up.isChecked()? 8: 0) |
                                (down.isChecked()? 16: 0);
                if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    mouse[0] = Math.round(event.getX());
                    mouse[1] = Math.round(event.getY());
                    mouse[2] = buttons;
                    mainActivity.setMouse(mouse);
                } else if (event.getAction() == MotionEvent.ACTION_MOVE) {
                    mouse[0] = Math.round(event.getX());
                    mouse[1] = Math.round(event.getY());
                    mouse[2] = buttons;
                    mainActivity.setMouse(mouse);
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    mouse[0] = Math.round(event.getX());
                    mouse[1] = Math.round(event.getY());
                    mouse[2] = 0;
                    mainActivity.setMouse(mouse);
                }
                return true;
            }
        });
        bmp = Bitmap.createBitmap(screenWidth, screenHeight, Bitmap.Config.ARGB_8888);
        new Thread(new Runnable() {
            private long last = 0;
            private long lastcb = 0;

            @Override
            public void run() {
                while (true) {
                    if ((SystemClock.currentThreadTimeMillis() - last) > 250) {
                        mainActivity.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                MySurfaceView.this.invalidate();
                            }
                        });
                        last = SystemClock.currentThreadTimeMillis();
                    }
                    if ((SystemClock.currentThreadTimeMillis() - lastcb) > 1500) {
                        new Thread(new Runnable() {
                            @Override
                            public void run() {
                                String s = mainActivity.getSnarf();
                                ClipboardManager cm = (ClipboardManager) mainActivity.getApplicationContext().getSystemService(Context.CLIPBOARD_SERVICE);
                                if (cm != null) {
                                    ClipData cd = cm.getPrimaryClip();
                                    String t = "";
                                    if (cd != null)
                                        t = cd.getItemAt(0).coerceToText(mainActivity.getApplicationContext()).toString();
                                    if (cd == null || !t.equals(s)) {
                                        cd = ClipData.newPlainText(null, s);
                                        cm.setPrimaryClip(cd);
                                    }
                                }
                            }
                        }).start();
                        lastcb = SystemClock.currentThreadTimeMillis();
                    }
                    try {
                        // TODO EBC seconds and millis?
                        Thread.sleep(0, 5000);
                    } catch(Exception e) {
                    }
                }
            }
        }).start();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        IntBuffer intBuffer = ByteBuffer.wrap(mainActivity.getScreenData()).asIntBuffer();
        int []ints = new int[intBuffer.remaining()];
        intBuffer.get(ints);
        bmp.setPixels(ints, 0, screenWidth, 0, 0, screenWidth, screenHeight);
        canvas.save();
        canvas.scale(ws, hs);
        canvas.drawBitmap(bmp, 0, 0, paint);
        canvas.restore();
    }

    protected class Listener implements ClipboardManager.OnPrimaryClipChangedListener {
        public void onPrimaryClipChanged() {
            ClipboardManager cm = (ClipboardManager)mainActivity.getApplicationContext().getSystemService(Context.CLIPBOARD_SERVICE);
            if (cm != null) {
                ClipData cd = cm.getPrimaryClip();
                if (cd != null)
                    mainActivity.setSnarf((String) (cd.getItemAt(0).coerceToText(mainActivity.getApplicationContext()).toString()));
            }
        }
    }
}
