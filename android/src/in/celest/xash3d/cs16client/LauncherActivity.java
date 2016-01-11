/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/
package in.celest.xash3d.cs16client;

import android.app.Activity;
import android.app.AlertDialog;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.content.Intent;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.CompoundButton;
import android.widget.ArrayAdapter;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.content.SharedPreferences;
import android.util.Log;

import com.google.android.gms.ads.*;

import java.io.File;

import in.celest.xash3d.cs16client.R;

public class LauncherActivity extends Activity {
	public final static String TAG = "LauncherActivity";
	
	static EditText cmdArgs;
	static SharedPreferences mPref;
	static Spinner mServerSpinner;
	static AdView mAdView;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_launcher);
		mPref          = getSharedPreferences("mod", 0);
		cmdArgs        = (EditText)findViewById(R.id.cmdArgs);
		mServerSpinner = (Spinner) findViewById(R.id.serverSpinner);
		mAdView        = (AdView)  findViewById(R.id.adView);

		cmdArgs.setText(mPref.getString("argv","-dev 5 -log"));
		ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this,
			R.array.avail_servers, android.R.layout.simple_spinner_item);
		adapter.setDropDownViewResource(android.R.layout.simple_spinner_item);
		mServerSpinner.setAdapter(adapter);
		mServerSpinner.setSelection(mPref.getInt("serverSpinner", 0));

		AdRequest adRequest = new AdRequest.Builder()
			.addTestDevice(AdRequest.DEVICE_ID_EMULATOR)
			.addTestDevice("B1F9AE0E2DC2387F53BE815077840D9B")
			.build();
		mAdView.loadAd(adRequest);
	}

	public void startXash(View view)
	{
		SharedPreferences.Editor editor = mPref.edit();
		String argv = cmdArgs.getText().toString();

		editor.putString("argv", argv);
		editor.putInt("serverSpinner", mServerSpinner.getSelectedItemPosition());
		editor.commit();
		editor.apply();

		switch(mServerSpinner.getSelectedItemPosition())
		{
		case 0:
			argv = argv + " -dll censored";
			break;
		case 1:
			// Engine will load libserver.so by himself
			break;
		case 2:
			String fullPath = getFilesDir().getAbsolutePath().replace("/files","/lib");
			File yapb_hardfp = new File( fullPath + "/libyapb_hardfp.so" );
			File yapb = new File( fullPath + "/libyapb.so" );
			if( yapb_hardfp.exists() && !yapb_hardfp.isDirectory() )
				argv = argv + " -dll " + yapb_hardfp.getAbsolutePath();
			else if( yapb.exists() && !yapb.isDirectory() )
				argv = argv + " -dll " + yapb.getAbsolutePath();
			else
			{
				Log.v(TAG, "YaPB not found!");
				AlertDialog.Builder notFoundDialogBuilder = new AlertDialog.Builder(this);
				notFoundDialogBuilder.setMessage(R.string.not_found_msg)
					.setTitle(R.string.not_found_title);
				notFoundDialogBuilder.create();
				return;
			}
			break;
		case 3:
		case 4:
			AlertDialog.Builder notImplementedDialogBuilder = new AlertDialog.Builder(this);
			notImplementedDialogBuilder.setMessage(R.string.not_implemented_msg)
				.setTitle(R.string.not_implemented_title);
			notImplementedDialogBuilder.create();
			return;
		}
		
		Intent intent = new Intent();
		intent.setAction("in.celest.xash3d.START");
		intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

		if(cmdArgs.length() != 0)
			intent.putExtra("argv", argv);
		intent.putExtra("gamedir", "cstrike");
		intent.putExtra("gamelibdir", getFilesDir().getAbsolutePath().replace("/files","/lib"));
		startActivity(intent);
	}

	@Override
	public void onResume() {
	super.onResume();
	if(mAdView != null)
		mAdView.resume();
	}

	@Override
	public void onDestroy() {
	if(mAdView != null)
		mAdView.destroy();

	super.onDestroy();
	}

	@Override
	public void onPause() {
	if(mAdView != null)
		mAdView.pause();
	
	super.onPause();
	}
}
