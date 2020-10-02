package org.echoline.drawterm;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Resources;
import android.graphics.Point;
import android.os.Bundle;
import android.os.Environment;

import android.app.Activity;

import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.view.WindowManager;
import android.view.WindowMetrics;
import android.view.Surface;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.content.ClipData;
import android.content.ClipboardManager;

import java.io.File;
import java.util.Map;

public class MainActivity extends Activity {
	private Map<String, ?> map;
	private MainActivity mainActivity;

	static {
		System.loadLibrary("drawterm");
	}

	public void serverView(View v) {
		setContentView(R.layout.server_main);
		serverButtons();

		String s = (String)map.get(((TextView)v).getText().toString());
		String []a = s.split("\007");

		((EditText)findViewById(R.id.cpuServer)).setText((String)a[0]);
		((EditText)findViewById(R.id.authServer)).setText((String)a[1]);
		((EditText)findViewById(R.id.userName)).setText((String)a[2]);
		((EditText)findViewById(R.id.passWord)).setText((String)a[3]);
	}

	public void populateServers(Context context) {
		ListView ll = findViewById(R.id.servers);
		ArrayAdapter<String> la = new ArrayAdapter<String>(this, R.layout.item_main);
		SharedPreferences settings = getSharedPreferences("DrawtermPrefs", 0);
		map = (Map<String, ?>)settings.getAll();
		String key;
		Object []keys = map.keySet().toArray();
		for (int i = 0; i < keys.length; i++) {
			key = (String)keys[i];
			la.add(key);
		}
		ll.setAdapter(la);
	}

	public void serverButtons() {
		Button button = (Button)findViewById(R.id.save);
		button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				String cpu = ((EditText)findViewById(R.id.cpuServer)).getText().toString();
				String auth = ((EditText)findViewById(R.id.authServer)).getText().toString();
				String user = ((EditText)findViewById(R.id.userName)).getText().toString();
				String pass = ((EditText)findViewById(R.id.passWord)).getText().toString();

				SharedPreferences settings = getSharedPreferences("DrawtermPrefs", 0);
				SharedPreferences.Editor editor = settings.edit();
				editor.putString(user + "@" + cpu + " (auth="  + auth + ")", cpu + "\007" + auth + "\007" + user + "\007" + pass);
				editor.commit();
			}
		});

		button = (Button) findViewById(R.id.connect);
		button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(final View view) {
				String cpu = ((EditText)findViewById(R.id.cpuServer)).getText().toString();
				String auth = ((EditText)findViewById(R.id.authServer)).getText().toString();
				String user = ((EditText)findViewById(R.id.userName)).getText().toString();
				String pass = ((EditText)findViewById(R.id.passWord)).getText().toString();

				Resources res = getResources();
				DisplayMetrics dm = res.getDisplayMetrics();

				int wp = dm.widthPixels;
				int hp = dm.heightPixels;

				setContentView(R.layout.drawterm_main);

				int rid = res.getIdentifier("navigation_bar_height", "dimen", "android");
				if (rid > 0) {
					hp -= res.getDimensionPixelSize(rid);
				}
				LinearLayout ll = findViewById(R.id.mouseButtons);
				hp -= ll.getHeight();

				int w = (int)(wp * (160.0/dm.xdpi));
				int h = (int)(hp * (160.0/dm.ydpi));
				float ws = (float)wp/w;
				float hs = (float)hp/h;
				// only scale up
				if (ws < 1) {
					ws = 1;
					w = wp;
				}
				if (hs < 1) {
					hs = 1;
					h = hp;
				}

				MySurfaceView mView = new MySurfaceView(mainActivity, w, h, ws, hs);
				mView.getHolder().setFixedSize(w, h);

				LinearLayout l = findViewById(R.id.dlayout);
				l.addView(mView, 1, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT));

				DrawTermThread t = new DrawTermThread(cpu, auth, user, pass, mainActivity);
				t.start();
			}
		});
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		File dir = Environment.getStorageDirectory();

		mainActivity = this;
		setObject();
		setContentView(R.layout.activity_main);
		populateServers(this);

		View fab = findViewById(R.id.fab);
		fab.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				setContentView(R.layout.server_main);
				serverButtons();
			}
		});
	}

	@Override
	public void onBackPressed()
	{
	}

	public void setClipBoard(String str) {
		ClipboardManager cm = (ClipboardManager)getApplicationContext().getSystemService(Context.CLIPBOARD_SERVICE);
		if (cm != null) {
			ClipData cd = ClipData.newPlainText(null, str);
			cm.setPrimaryClip(cd);
		}
	}

	public String getClipBoard() {
		ClipboardManager cm = (ClipboardManager)getApplicationContext().getSystemService(Context.CLIPBOARD_SERVICE);
		if (cm != null) {
			ClipData cd = cm.getPrimaryClip();
			if (cd != null)
				return (String)(cd.getItemAt(0).coerceToText(mainActivity.getApplicationContext()).toString());
		}
		return "";
	}

	public native void dtmain(Object[] args);
	public native void setPass(String arg);
	public native void setWidth(int arg);
	public native void setHeight(int arg);
	public native void setWidthScale(float arg);
	public native void setHeightScale(float arg);
	public native void setDTSurface(Surface surface);
	public native void setMouse(int[] args);
	public native void setObject();
}
